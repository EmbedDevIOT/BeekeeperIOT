#include "Config.h"
#include "FileConfig.h"

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5        /* Time ESP32 will go to sleep (in seconds) */


#define EB_CLICK_TIME 100 // таймаут ожидания кликов (кнопка)

#define DISP_TIME (tmrMin == 10 && tmrSec == 0)

// GPIO PINs
#define SET_PIN 18        // кнопкa Выбор
#define PL_PIN 19         // кнопкa Плюс
#define MN_PIN 5          // кнопкa Минус
#define RELAY 23          // Реле
#define DS_SNS 4          // ds18b20
#define TX_PIN 17         // tx
#define RX_PIN 16         // rx
#define HX_DT 25          // HXT
#define HX_CLK 26         // rx

/*
I2C device found at address 0x3C !
I2C device found at address 0x68 !
I2C device found at address 0x76 !
*/
#define BME_ADR 0x76
#define OLED_ADR 0x3C
#define RTC_ADR 0x68

GlobalConfig Config;
SNS sensors;
SYTM System;
DateTime Clock;

uint16_t tmrSec = 0;
uint16_t tmrMin = 0;

// #define OLED_SPI_SPEED 8000000ul
RTC_DATA_ATTR int bootCount = 0;

GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> disp;
HX711 scale; //
SoftwareSerial SIM800(TX_PIN, RX_PIN);
// MicroDS18B20<DS_SNS> ds;
// OneWire DS(DS_SNS);
MicroDS3231 RTC; // 0x68
Button btUP(PL_PIN, INPUT_PULLUP);
Button btSET(SET_PIN, INPUT_PULLUP);
Button btDWN(MN_PIN, INPUT_PULLUP);

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C BUS_

void I2C_Scanning(void);
void StartingInfo();
void DisplayUpd();
void ButtonHandler();
void BeekeeperConroller();
void Task500ms();
void Task1000ms();

/* Method to print the reason by which ESP32
has been awaken from sleep
*/

void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

void setup()
{
  Config.firmware = "0.4";

  Serial.begin(UARTSpeed);
  Serial.println("Beekeeper");
  Serial.printf("firmware: %s", Config.firmware);
  Serial.println();
  Wire.begin();
  delay(20);
  I2C_Scanning();
  delay(2000);
  byte errSPIFFS = SPIFFS.begin(true);
  Serial.println(F("SPIFFS...init"));
  LoadConfig();

  byte errRTC = RTC.begin();
  Clock = RTC.getTime();
  Serial.println(F("RTC...init"));

  scale.begin(HX_DT, HX_CLK); // HX
  disp.init();

  bool status;
  status = bme.begin(0x76);
  if (!status)
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1)
      ;
  }
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1,
                  Adafruit_BME280::SAMPLING_X1,
                  Adafruit_BME280::SAMPLING_X1,
                  Adafruit_BME280::FILTER_OFF);

  bme.takeForcedMeasurement();
  uint32_t Pressure = bme.readPressure();
  delay(20);

  pinMode(RELAY,OUTPUT);
  digitalWrite(RELAY,LOW);

  StartingInfo();
  delay(1500);
  disp.clear();
  disp.invertText(false);
}

void loop()
{
  if (System.DispState)
  {
    DisplayUpd();
  }
  else
    disp.setPower(false);

  ButtonHandler();

  Task500ms();
  Task1000ms();

  // BeekeeperConroller();
  // task_1000();
  // if (scale.is_ready())
  // {
  //   scale.set_scale();
  //   Serial.println("Tare... remove any weights from the scale.");
  //   delay(5000);
  //   scale.tare();
  //   Serial.println("Tare done...");
  //   Serial.print("Place a known weight on the scale...");
  //   delay(5000);
  //   long reading = scale.get_units(10);
  //   Serial.print("Result: ");
  //   Serial.println(reading);
  // }
  // else
  // {
  //   Serial.println("HX711 not found.");
  // }
  // delay(1000);
}

void StartingInfo()
{
  disp.clear();     // очистка
  disp.setScale(2); // масштаб текста (1..4)
  // oled.home();      // курсор в 0,0
  disp.setCursor(10, 0);
  disp.print("Beekeeper");
  delay(1000);
  disp.setScale(1);
  // курсор на начало 3 строки
  disp.setCursor(0, 3);
  disp.print("starting...");

  disp.clear();
  disp.update();
  byte textPos1 = 8;
  byte textPos2 = 32;

  disp.createBuffer(5, 0, 66, textPos2 + 8 + 2);

  disp.roundRect(5, textPos1 - 4, 65, textPos1 + 8 + 2, OLED_STROKE);
  disp.setCursorXY(10, textPos1);
  disp.print("SET MODE");

  disp.roundRect(5, textPos2 - 4, 65, textPos2 + 8 + 2, OLED_FILL);
  disp.setCursorXY(10, textPos2);
  disp.invertText(true);
  disp.print("LOL KEK");

  disp.sendBuffer();
  disp.update();
}

void ButtonHandler()
{
  uint16_t value = 0;
  static bool btnst = false;

  btSET.tick();
  btUP.tick();
  btDWN.tick();

  while (btSET.busy())
  {
    btSET.tick();
    if (btSET.click())
    {
      Serial.println("State: BTN_ SET_ Click");
      System.RelayState = !System.RelayState;
    }
  }

  while (btUP.busy())
  {
    btUP.tick();
    if (btUP.hasClicks(1))
    {
      Serial.println("State: BTN_UP_ Click");
      disp.setPower(true);
      System.DispState = true;
      tmrMin = 0;
      tmrSec = 0;
    }
  }

  while (btDWN.busy())
  {
    btDWN.tick();
    if (btDWN.hasClicks(1))
    {
      Serial.println("State: BTN_DWN_ Click");
      disp.setPower(false);
    }
  }
}

// Getting Time and Date from RTC
void GetClock()
{
  // System.hour = RTC.getHours(h12, PM);
  // System.min = Clock.getMinute();
  // year = Clock.getYear();
  // month = Clock.getMonth(Century);
  // date = Clock.getDate();
}

// Get Data from BME Sensor
void GetBme()
{
  bme.takeForcedMeasurement();
  sensors.bmeT = bme.readTemperature();
  sensors.bmeH = bme.readHumidity();
  sensors.bmeP = (float)bme.readPressure() * 0.00750062;
}

String waitResponse()
{
  String _resp = "";
  long _timeout = millis() + 10000;
  while (!SIM800.available() && millis() < _timeout)
  {
  };
  if (SIM800.available())
  {
    _resp = SIM800.readString();
  }
  else
  {
    Serial.println("Timeout...");
  }
  return _resp;
}

String sendATCommand(String cmd, bool waiting)
{
  String _resp = "";
  Serial.println(cmd);
  SIM800.println(cmd);
  if (waiting)
  {
    _resp = waitResponse();
    if (_resp.startsWith(cmd))
    {
      _resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
    }
    Serial.println(_resp);
  }
  return _resp;
}

void sendSMS(String phone, String message)
{
  sendATCommand("AT+CMGS=\"" + phone + "\"", true);
  sendATCommand(message + "\r\n" + (String)((char)26), true);
}

void DisplayUpd()
{
  char dispbuf[30];

  static uint32_t tmr;

  if (millis() - tmr >= 50)
  {
    tmr = millis();
    sprintf(dispbuf, "%02d:%02d:%02d", Clock.hour, Clock.minute, Clock.second);
    disp.setScale(2);
    disp.setCursor(15, 0);
    disp.print(dispbuf);
    disp.update();
    
    sprintf(dispbuf, "Вес: %0.1f кг", 10.3);
    disp.setScale(2);
    disp.setCursor(5, 2);
    disp.print(dispbuf);
    disp.update();

    // sprintf(dispbuf, "T1:%0.1fC T2:%0.1fC H:%0.1f% P:%3dkPa", sensors.dsT, sensors.bmeT, sensors.bmeH, sensors.bmeP);
    // disp.setScale(1);
    // disp.setCursor(5, 2);
    // disp.print(dispbuf);
    // disp.update();
  }
}

void Task500ms()
{
  static uint32_t tmr500;

  if (millis() - tmr500 >= 500)
  {
    tmr500 = millis();
    Clock = RTC.getTime();

    if(System.RelayState){
      digitalWrite(RELAY,ENABLE);
    }
    else digitalWrite(RELAY, DISABLE);

  }
}

void Task1000ms()
{
  char serbuf[30];

  static uint32_t tmr1000;

  if (millis() - tmr1000 >= 1000)
  {
    tmr1000 = millis();
    // GetBme();
    if (System.DispState)
    {
      if (tmrSec < 59)
      {
        tmrSec++;
      }
      else
      {
        tmrSec = 0;
        tmrMin++;
      }
      // sprintf(serbuf, "T:%02d:%02d", tmrMin, tmrSec);
      // Serial.print(serbuf);
      // Serial.println();
    }

    if DISP_TIME
    {
      // disp.setPower(false);
      System.DispState = false;
      Serial.println("TimeOut: Display - OFF");
      tmrMin = 0;
      tmrSec = 0;
    }
  }
}

/**************************************** Scanning I2C bus *********************************************/
void I2C_Scanning(void)
{
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 8; address < 127; address++)
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");

      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknow error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  // delay(5000);
}
/*******************************************************************************************************/