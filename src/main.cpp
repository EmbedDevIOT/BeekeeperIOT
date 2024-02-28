#include "Config.h"
#include "FileConfig.h"
//=======================================================================

//========================== DEFINITIONS ================================
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5        /* Time ESP32 will go to sleep (in seconds) */

#define EB_CLICK_TIME 100 // Button timeout
#define DISP_TIME (tmrMin == 10 && tmrSec == 0)
#define ITEMS 6 // Main Menu Items

// GPIO PINs
#define SET_PIN 18 // кнопкa Выбор
#define PL_PIN 19  // кнопкa Плюс
#define MN_PIN 5   // кнопкa Минус
#define RELAY 23   // Реле
#define DS_SNS 4   // ds18b20
#define BAT 34     // Аккумулятор
#define TX_PIN 17  // SIM800_TX
#define RX_PIN 16  // SIM800_RX
#define HX_DT 25   // HX711_DT
#define HX_CLK 26  // HX711_CLK

// I2C Adress
#define BME_ADR 0x76
#define OLED_ADR 0x3C
#define RTC_ADR 0x68
//=======================================================================

//============================== STRUCTURES =============================
GlobalConfig Config;
SNS sensors;
SYTM System;
DateTime Clock;
//=======================================================================

//============================ GLOBAL VARIABLES =========================
float temperature = 0.0;
uint16_t tmrSec = 0;
uint16_t tmrMin = 0;
uint8_t disp_ptr = 0;
uint8_t address[8]; // Создаем массив для адреса

RTC_DATA_ATTR int bootCount = 0;
//================================ OBJECTs =============================
#define OLED_SOFT_BUFFER_64 // Буфер на стороне МК
GyverOLED<SSD1306_128x64> disp;
HX711 scale; //
// Serial1 SIM800(TX_PIN, RX_PIN);
MicroDS3231 RTC; // 0x68
GyverBME280 bme; // 0x76
Button btUP(PL_PIN, INPUT_PULLUP);
Button btSET(SET_PIN, INPUT_PULLUP);
Button btDWN(MN_PIN, INPUT_PULLUP);

// Themperature sensor Dallas DS18b20
OneWire oneWire(DS_SNS);
DallasTemperature ds18b20(&oneWire);

//=======================================================================

//================================ PROTOTIPs =============================
void I2C_Scanning(void);
void StartingInfo(void);
void DisplayUPD(void);
void ButtonHandler(void);
void BeekeeperConroller(void);
void Task500ms(void);
void Task1000ms(void);
void ShowMainMenu(uint8_t item);
void printPointer(uint8_t pointer);
void MenuControl(void);
void GetBatVoltage(void);
void GetDSData(void);
void GetBMEData(void);
void ShowDBG(void);
//=======================================================================

void setup()
{
  Config.firmware = "0.6";

  Serial.begin(UARTSpeed);
  Serial1.begin(9600);

  Serial.println("Beekeeper");
  Serial.printf("firmware: %s", Config.firmware);
  Serial.println();
  Wire.begin();
  delay(20);
  I2C_Scanning();
  delay(2000);
  // byte errSPIFFS = SPIFFS.begin(true);
  // Serial.println(F("SPIFFS...init"));
  // LoadConfig();

  byte errRTC = RTC.begin();
  Clock = RTC.getTime();
  Serial.println(F("RTC...init"));

  scale.begin(HX_DT, HX_CLK); // HX
  disp.init();
  disp.setContrast(255);

  bme.begin(0x76);
  ds18b20.begin();
  delay(20);

  pinMode(BAT, INPUT);
  // pinMode(RELAY, OUTPUT);
  // digitalWrite(RELAY, LOW);

  StartingInfo();
  delay(1500);
  disp.clear();
}

void loop()
{
  Task500ms();
  Task1000ms();

  if (System.DispState)
  {
    DisplayUPD();
  }
  else
    disp.setPower(false);

  ButtonHandler();

  // BeekeeperConroller();

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
  disp.printf("fw:%s", Config.firmware);

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
      System.DispMenu == 0 ? System.DispMenu = 1 : System.DispMenu = 0;
      disp.clear();
      disp.home();
      // System.RelayState = !System.RelayState;
    }
  }

  while (btUP.busy())
  {
    btUP.tick();
    if (btUP.click())
    {
      Serial.println("State: BTN_UP_ Click");
      disp.setPower(true);
      System.DispState = true;
      tmrMin = 0;
      tmrSec = 0;
      // Уменьшаем указатель на 1, и если он стал меньше 1, присваиваем указателю ITEM - 1
      disp_ptr = constrain(disp_ptr - 1, 0, ITEMS - 1); // Двигаем указатель в пределах дисплея

      Serial.printf("ptr:%d", disp_ptr);
      Serial.println();
    }
  }

  while (btDWN.busy())
  {
    btDWN.tick();
    if (btDWN.click())
    {
      Serial.println("State: BTN_DWN_ Click");

      disp_ptr = constrain(disp_ptr + 1, 0, ITEMS - 1);

      Serial.printf("ptr:%d", disp_ptr);
      Serial.println();
    }
  }
}

// Get Data from BME Sensor
void GetBmeData()
{
  sensors.bmeT = bme.readTemperature();
  sensors.bmeH = bme.readHumidity() + sensors.bmeHcal;
  sensors.bmeP_hPa = bme.readPressure();
  // sensors.bmeP_hPa = sensors.bmeP_hPa / 100.0F;
  sensors.bmeP_mmHg = pressureToMmHg(sensors.bmeP_hPa);
  sensors.bmeA = pressureToAltitude(sensors.bmeP_hPa);

  // Serial.print("Temperature: ");
  // Serial.print(bme.readTemperature()); // Выводим темперутуру в [*C]
  // Serial.println(" *C");

  // Serial.print("Humidity: ");
  // Serial.print(bme.readHumidity()); // Выводим влажность в [%]
  // Serial.println(" %");

  // float pressure = bme.readPressure(); // Читаем давление в [Па]
  // Serial.print("Pressure: ");
  // Serial.print(pressure / 100.0F); // Выводим давление в [гПа]
  // Serial.print(" hPa , ");
  // Serial.print(pressureToMmHg(pressure)); // Выводим давление в [мм рт. столба]
  // Serial.println(" mm Hg");
  // Serial.print("Altitide: ");
  // Serial.print(pressureToAltitude(pressure)); // Выводим высоту в [м над ур. моря]
  // Serial.println(" m");
  // Serial.println("");
}

// Get Data from DS18B20 Sensor
void GetDSData()
{
  ds18b20.requestTemperatures();
  sensors.dsT = ds18b20.getTempCByIndex(0);
}

// Get Data from HX711
void GetW()
{  
  scale.set_scale(sensors.calib);
  sensors.units = scale.get_units(), 10;
  sensors.grams = (sensors.units * 0.035274);
}

void DisplayUPD()
{

  static uint32_t tmr;

  if (millis() - tmr >= 50)
  {
    tmr = millis();
    ShowMainMenu(System.DispMenu);
  }
}

void Task500ms()
{
  static uint32_t tmr500;

  if (millis() - tmr500 >= 500)
  {
    tmr500 = millis();
    Clock = RTC.getTime();

    if (System.RelayState)
    {
      digitalWrite(RELAY, ENABLE);
    }
    else
      digitalWrite(RELAY, DISABLE);
  }
}

void Task1000ms()
{
  char serbuf[30];

  static uint32_t tmr1000;

  if (millis() - tmr1000 >= 1000)
  {
    tmr1000 = millis();

    GetBatVoltage();
    GetBmeData();
    GetDSData();

    ShowDBG();

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
/******************************************* MAIN_MENU *************************************************/
void ShowMainMenu(uint8_t item)
{
  switch (item)
  {
  case Menu:
    disp.clear(); // Очищаем буфер
    disp.home();  // Курсор в левый верхний угол
    disp.setScale(1);
    disp.print // Вывод всех пунктов
        (F(
            "  Время:\r\n"
            "  Дата:\r\n"
            "  Калибровка:\r\n"
            "  Оповещения:\r\n"
            "  Аккумулятор:\r\n"
            "  Выход:\r\n"));

    printPointer(disp_ptr); // Вывод указателя
    disp.update();          // Выводим кадр на дисплей
    break;

  case Action:
    char dispbuf[30];

    sprintf(dispbuf, "%02d:%02d:%02d", Clock.hour, Clock.minute, Clock.second);
    disp.setScale(2);
    disp.setCursor(15, 0);
    disp.print(dispbuf);
    disp.update();

    sprintf(dispbuf, "W:%0.1f T1:%0.1f", 10.3, sensors.dsT);
    disp.setScale(1);
    disp.setCursor(5, 2);
    disp.print(dispbuf);
    disp.update();

    // sprintf(dispbuf, "T1:%0.1fC T2:%0.1fC H:%0.1f% P:%3dkPa", sensors.dsT, sensors.bmeT, sensors.bmeH, sensors.bmeP);
    break;

  case Time:

    break;
  case Date:

    break;
  case Calib:

    break;
  case Notifycation:

    break;
  case Battery:

    break;
  case IDLE:

    break;
  default:
    break;
  }
}
/*******************************************************************************************************/
void printPointer(uint8_t pointer)
{
  disp.setCursor(0, pointer);
  disp.print(">");
}

void MenuControl(void)
{
  disp.clear();
  disp.home();
  disp.print(F("Press OK to return"));
  disp.update();
  while (1)
  {
    btSET.tick();
    if (btSET.click())
      return; // return возвращает нас в предыдущее меню
  }
}

String waitResponse()
{
  String _resp = "";
  long _timeout = millis() + 10000;
  // while (!SIM800.available() && millis() < _timeout)
  // {
  // };
  // if (SIM800.available())
  // {
  //   _resp = SIM800.readString();
  // }
  // else
  // {
  //   Serial.println("Timeout...");
  // }
  return _resp;
}

String sendATCommand(String cmd, bool waiting)
{
  String _resp = "";
  Serial.println(cmd);
  // SIM800.println(cmd);
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

void GetBatVoltage()
{
  uint32_t _mv = 0;
  // 1250 = 10.8
  // 1380 = 12.82

  for (uint8_t i = 0; i <= 4; i++)
  {
    _mv += analogReadMilliVolts(BAT);
  }
  _mv = _mv / 4;

  if (_mv == 0 || _mv < 1250)
  {
    _mv = 0;
  }
  else if (_mv >= 1250 && _mv <= 1480)
  {
    _mv = map(_mv, 1250, 1480, 10, 100);
  }
  else if (_mv > 1480)
  {
    _mv = 1480;
    _mv = map(_mv, 1250, 1480, 10, 100);
  }
  sensors.voltage = _mv;
}

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

void ShowDBG()
{
  char message[50];
  uint8_t H = 0;

  Serial.println(F("!!!!!!!!!!!!!!  DEBUG INFO  !!!!!!!!!!!!!!!!!!"));

  sprintf(message, "TimeRTC: %02d:%02d:%02d", Clock.hour, Clock.minute, Clock.second);
  Serial.println(message);
  sprintf(message, "T_DS:%0.2f *C", sensors.dsT);
  Serial.println(message);
  sprintf(message, "T_BME:%0.2f *C | H_BME:%0d % | P_BHE:%d", sensors.bmeT, (int)sensors.bmeH, (int)sensors.bmeP_mmHg);
  Serial.println(message);
  sprintf(message, "WEIGHT: %0.2fg | W_EEP: %0.2fg ", sensors.grams, sensors.g_eeprom);
  Serial.println(message);
  sprintf(message, "BAT: %003d", sensors.voltage);
  Serial.println(message);

  Serial.println(F("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
  Serial.println();
}
