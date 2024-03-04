#include "Config.h"
#include "FileConfig.h"
//=======================================================================

//========================== DEFINITIONS ================================
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5        /* Time ESP32 will go to sleep (in seconds) */

// #define EB_CLICK_TIME 100 // Button timeout
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
Flag ST;
//=======================================================================

//============================ GLOBAL VARIABLES =========================
String _response = "";

uint8_t task_counter = 0, task_cnt_10S = 0;
float Calibration_Factor_Of_Load_cell = 23850; //-31;

uint32_t now;

uint16_t tmrSec = 0;
uint16_t tmrMin = 0;
uint8_t disp_ptr = 0;
uint8_t address[8]; // Создаем массив для адреса

String currStr = "";
boolean isStringMessage = false;

RTC_DATA_ATTR int bootCount = 0;
//================================ OBJECTs =============================
#define OLED_SOFT_BUFFER_64 // Буфер на стороне МК
GyverOLED<SSD1306_128x64> disp;
GyverOS<3> os;
HX711 scale; //
HardwareSerial SIM800(1);
// Serial1 SIM800(TX_PIN, RX_PIN);
MicroDS3231 RTC; // 0x68
GyverBME280 bme; // 0x76
Button btUP(PL_PIN, INPUT_PULLUP);
Button btSET(SET_PIN, INPUT_PULLUP);
Button btDWN(MN_PIN, INPUT_PULLUP);
VirtButton btVirt;

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
void Task1MIN(void);
void DisplayHandler(uint8_t item);
void printPointer(uint8_t pointer);
String waitResponse(void);
String sendATCommand(String cmd, bool waiting);
void sendSMS(String phone, String message);
void MenuControl(void);
void GetBatVoltage(void);
void IncommingRing(void);
void GetDSData(void);
void GetBMEData(void);
void GetWeight(void);
void ShowDBG(void);
void task0(void);
void task1(void);
void task2(void);
void task3(void);
//=======================================================================

//=======================   I2C Scanner     =============================
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
}
//=======================================================================

//=======================================================================
void StartingInfo()
{
  disp.clear();     // очистка
  disp.setScale(2); // масштаб текста (1..4)

  disp.setCursor(10, 3);
  disp.print("Beekeeper");
  Serial.println("Beekeeper");
  disp.setScale(1);

  // курсор на начало 3 строки
  disp.setCursor(20, 7);
  disp.printf("firmware: %s", Config.firmware);
  Serial.printf("firmware: %s", Config.firmware);
  Serial.println();
  disp.update();

  delay(1000);
  disp.clear();
}
//=======================================================================

//=======================       SETUP     =============================
void setup()
{
  Config.firmware = "0.8.6";

  Serial.begin(UARTSpeed);
  Serial1.begin(MODEMSpeed);
  Wire.begin();

  // OLED INIT
  disp.init();
  disp.setContrast(255);
  disp.clear();
  disp.update();
  Serial.println(F("OLED...Done"));
  StartingInfo();
  // delay(1500);

  // HX711 Init
  scale.begin(HX_DT, HX_CLK);

  // EEPROM Init
  // ADRs: 0 - Calibration 4 - State_Calibration (Done or False) 5 - FirstStart State
  EEPROM.begin(10);

  // Если Весы не откалиброваны
  // if (ST.Calibration != CALL_DONE)
  disp.setScale(2); // масштаб текста (1..4)
  disp.setCursor(13, 3);
  disp.print("Загрузка");
  disp.update();

  Serial.print("Reading Calibration State to EEPROM: ");
  ST.Calibration = EEPROM.read(4);
  Serial.print(ST.Calibration);
  Serial.println();

  // if (EEPROM.read(4) != EEP_DONE)
  if (ST.Calibration != EEP_DONE)
  {
    bool st = true;
    scale.set_scale();
    Serial.println("1. Tare... remove any weights from the scale...and PRESS to SET button");

    disp.clear();
    disp.setScale(2); // масштаб текста (1..4)
    disp.setCursor(10, 2);
    disp.print("#Калибровка#");
    disp.update();

    disp.clear();
    disp.setScale(1);
    disp.setCursor(1, 1);
    disp.print(F(
        "Освободите\r\n"
        "платформу и\r\n"
        "нажмите кнопку ОК\r\n"));
    disp.update();

    while (st)
    {
      btSET.tick();
      if (btSET.click())
      {
        st = false;
        scale.tare();
        Serial.println("Tare DONE...");
      }
    }

    disp.clear();
    disp.setScale(2); // масштаб текста (1..4)
    disp.setCursor(10, 2);
    disp.print("#Калибровка#");
    disp.update();

    disp.clear();
    disp.setScale(1);
    disp.setCursor(1, 1);
    disp.print(F(
        "Пометите груз весом\r\n"
        "1кг платформу и\r\n"
        "нажмите кнопку ОК\r\n"));
    disp.update();

    Serial.print("2. Place a known weight on the scale...and PRESS to SET button");
    st = true;
    while (st)
    {
      btSET.tick();
      if (btSET.click())
      {
        st = false;
        sensors.calib = scale.get_units(10);
        sensors.calib = sensors.calib / 1000;
      }
    }
    // sensors.calib = scale.get_units(10);
    // sensors.calib = sensors.calib / 1000;
    Serial.println("4.Zero factor: ");
    Serial.print(sensors.calib);
    Serial.println();

    disp.clear();
    disp.setScale(2); // масштаб текста (1..4)
    disp.setCursor(10, 2);
    disp.print(F(
        "#Калибровка# \r\n"
        " выполнена \r\n"));
    disp.update();

    Serial.println(F("-= Calibration Done =-"));
    ST.Calibration = EEP_DONE;
    EEPROM.put(0, sensors.calib);
    EEPROM.commit();
    EEPROM.put(4, EEP_DONE);
    // EEPROM.write(4, EEP_DONE);
    EEPROM.commit();
  }
  else
  {
    Serial.println("Reading Calibration factor to EEPROM");
    EEPROM.get(0, sensors.calib);
    // scale.set_scale(sensors.calib);
    Serial.printf("EEPROM: Calib: %f", sensors.calib);
    Serial.println();
  }

  delay(2000);

  EEPROM.get(5, ST.FirstStart);
  Serial.printf("EEPROM: StartST: %d", ST.FirstStart);
  Serial.println();

  // Первый запуск устройства. и калибровка ОК. Обнуление
  if (ST.FirstStart != EEP_DONE && ST.Calibration == EEP_DONE)
  {
    Serial.println(F("First Starting..Zeroing"));
    scale.set_scale(sensors.calib);
    scale.tare();

    ST.FirstStart = EEP_DONE;
    EEPROM.write(5, ST.FirstStart);
    EEPROM.commit();

    Serial.println(F("-=First Start Done=-"));
  }
  else
  {
    scale.set_scale(sensors.calib);
    Serial.println(F("Set_scale"));
    scale.tare();
    scale.power_down();
  }

  RTC.begin();
  // RTC INIT
  if (RTC.lostPower())
  {                            //  при потере питания
    RTC.setTime(COMPILE_TIME); // установить время компиляции
  }

  Clock = RTC.getTime();
  Serial.println(F("RTC...Done"));

  // BME and DS SENSOR INIT
  bme.begin(0x76);
  Serial.println(F("BME...Done"));
  ds18b20.begin();
  Serial.println(F("DS18b20...Done"));
  delay(20);

  pinMode(BAT, INPUT);
  // // pinMode(RELAY, OUTPUT);
  // // digitalWrite(RELAY, LOW);

  // SIM800 INIT
  // // delay(15000);
  // delay(2000);
  SIM800.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  sendATCommand("AT", true);
  sendATCommand("AT+CMGDA=\"DEL ALL\"", true);

  _response = sendATCommand("AT+CMGF=1;&W", true);
  _response = sendATCommand("AT+IFC=1, 1", true);
  _response = sendATCommand("AT+CPBS=\"SM\"", true);
  _response = sendATCommand("AT+CLIP=1", true); // Включаем АОН
  _response = sendATCommand("AT+CNMI=1,2,2,1,0", true);
  // delay(10);

  disp.clear();

  GetBatVoltage();
  GetBMEData();
  GetDSData();

  os.attach(0, task0, 5000);
  os.attach(1, task1, 500);
  os.attach(2, task2, 1000);
  os.attach(3, task3, 1000);
  // for(;;)
  // {
  //   if (SIM800.available()) {
  //   // Если есть данные для чтения из RS485
  //   char data = SIM800.read();
  //   Serial.print("Получили данные: ");
  //   Serial.println(data);
  //   }
  // }
}
//=======================================================================

//=======================================================================
void loop()
{
  os.tick();
  ButtonHandler();
  // IncommingRing();
  // Task500ms();
  // Task1000ms();
  // Task1MIN();
  //
  // BeekeeperConroller();
}

void ButtonHandler()
{
  uint16_t value = 0;
  static bool btnst = false;

  btSET.tick();
  btUP.tick();
  btDWN.tick();
  btVirt.tick(btUP, btDWN);

  if (btSET.click())
  {
    Serial.println("State: BTN_ SET_ Click");

    if (System.DispState == true)
    {
      if (System.DispMenu == Menu && disp_ptr == 0)
      {
        System.DispMenu = Time;
        Serial.println("Set Time");
      }

      if (System.DispMenu == Menu && disp_ptr == 1)
      {
        System.DispMenu = Date;
        Serial.println("Set Date");
      }

      if (System.DispMenu == Menu && disp_ptr == 2)
      {
        System.DispMenu = Calib;
        Serial.println("Set Calibration");
      }

      if (System.DispMenu == Menu && disp_ptr == 3)
      {
        System.DispMenu = Notifycation;
        Serial.println("Set Notifycation");
      }

      if (System.DispMenu == Menu && disp_ptr == 4)
      {
        System.DispMenu = Battery;
        Serial.println("Set Battery");
      }

      if (System.DispMenu == Menu && disp_ptr == 5)
      {
        System.DispMenu = Action;
        Serial.println("Exit");
      }
      // System.DispMenu == Menu ? System.DispMenu = Action : System.DispMenu = Menu;
    }
    else
    {
      disp.setPower(true);
      System.DispMenu = Action;
      System.DispState = true;
    }

    tmrMin = 0;
    tmrSec = 0;
    disp.clear();
    // System.RelayState = !System.RelayState;
  }

  if (btUP.click() || btUP.hold())
  {
    Serial.println("BTN_UP");
    tmrMin = 0;
    tmrSec = 0;
    if (System.DispMenu == Menu)
      disp_ptr = constrain(disp_ptr - 1, 0, ITEMS - 1);

    Serial.printf("ptr:%d", disp_ptr);
    Serial.println();
  }

  if (btVirt.click())
  {
    Serial.println("BTN_UP and BTN_DWN");
    System.DispMenu = SetZero;
    disp.clear();
  }

  if (btDWN.click() || btDWN.hold())
  {
    Serial.println("State: BTN_DWN_ Click");
    tmrMin = 0;
    tmrSec = 0;

    if (System.DispMenu == Menu)
      disp_ptr = constrain(disp_ptr + 1, 0, ITEMS - 1);

    Serial.printf("ptr:%d", disp_ptr);
    Serial.println();
  }

  if (btSET.hasClicks(2))
  {
    Serial.println("Has double cliks");
    Serial.println("---------------------");

    String sms = "Bec: ";

    sms += String(sensors.kg, 1);
    sms += " Kg";
    sms += "\n";
    sms += "B: ";
    sms += sensors.voltage;
    sms += " %";
    sms += "\n";
    sms += "T1: ";
    sms += String(sensors.dsT, 1);
    sms += " *C";
    sms += "\n";
    sms += "T2: ";
    sms += String(sensors.bmeT, 1);
    sms += " *C";
    sms += "\n";
    sms += "H: ";
    sms += sensors.bmeH;
    sms += " %";
    sms += "\n";
    sms += "Pr: ";
    sms += sensors.bmeP_mmHg;

    Serial.println(sms);
    Serial.println("---------------------");

    sendSMS(Config.phone, sms); 
  }
}
void IncommingRing()
{
  if (SIM800.available())
  {
    _response = waitResponse();
    _response.trim();
    Serial.println(_response);
    if (_response.startsWith("RING"))
    {
      int phoneindex = _response.indexOf("+CLIP: \"");
      String innerPhone = "";
      if (phoneindex >= 0)
      {
        phoneindex += 8;
        innerPhone = _response.substring(phoneindex, _response.indexOf("\"", phoneindex));
        Serial.println("Number: " + innerPhone);
        delay(500);
        sendATCommand("ATH", true);
        delay(500);

        String smska = "Bec: ";
        smska += String(sensors.kg, 1);
        smska += " Kg";
        smska += "\n";
        smska += "T1:";
        smska += String(sensors.dsT, 1);
        smska += "\n";
        smska += "T2";
        smska += String(sensors.bmeT, 1);
        smska += "\n";
        smska += "H:";
        smska += String(sensors.bmeH, 1);
        smska += "\n";
        smska += "Pr:";
        smska += String(sensors.bmeP_mmHg);

        Serial.println(smska);
        // sendSMS(innerPhone, smska); // смс
        // delay(2000);
      }
    }
  }
}
// Get Data from BME Sensor
void GetBMEData()
{
  sensors.bmeT = bme.readTemperature();
  sensors.bmeH = (int)bme.readHumidity() + sensors.bmeHcal;
  sensors.bmeP_hPa = bme.readPressure();
  // sensors.bmeP_hPa = sensors.bmeP_hPa / 100.0F;
  sensors.bmeP_mmHg = (int)pressureToMmHg(sensors.bmeP_hPa);
  // sensors.bmeA = pressureToAltitude(sensors.bmeP_hPa);
}

// Get Data from DS18B20 Sensor
void GetDSData()
{
  ds18b20.requestTemperatures();
  sensors.dsT = ds18b20.getTempCByIndex(0);
}

// Get Data from HX711
void GetWeight()
{
  // static uint32_t _tmr;
  // char msg[24];

  // if ((ST.Calibration == EEP_DONE) && millis() - _tmr >= 1000)
  // {
  //   _tmr = millis();
  scale.power_up();
  sensors.units = scale.get_units(10);
  scale.power_down();

  if (sensors.units < 0)
  {
    sensors.units = 0.00;
  }
  sensors.kg = sensors.units / 1000;

  // sprintf(msg, "W: %0.1fg ", sensors.units);
  // Serial.println(msg);
  // Serial.print(sensors.kg, 1);
  // Serial.println();
  // }

  // scale.set_scale(sensors.calib);
  // sensors.units = scale.get_units(), 10;
  // if (sensors.units < 0)
  //   sensors.units = 0;
  // sensors.grams = (sensors.units * 0.035274);
}

void Task500ms()
{
  static uint32_t tmr500;

  if (millis() - tmr500 >= 500)
  {
    tmr500 = millis();
    Clock = RTC.getTime();

    GetWeight();

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

    task_counter++;

    if (ST.Calibration == EEP_DONE)
      // GetW();

      ShowDBG();

    if (System.DispState)
    {
      DisplayHandler(System.DispMenu);

      if (tmrSec < 59)
      {
        tmrSec++;
      }
      else
      {
        tmrSec = 0;
        tmrMin++;
        // disp.setPower(false);
      }
    }
    else
      disp.setPower(false);

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

void Task1MIN()
{
  if (task_counter == 60)
  {
    task_counter = 0;
    GetBatVoltage();
    GetBMEData();
    GetDSData();
  }
}

/******************************************* MAIN_MENU *************************************************/
void DisplayHandler(uint8_t item)
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
    disp.setCursor(20, 0);
    disp.print(dispbuf);

    sprintf(dispbuf, "%0.1f", sensors.kg);
    disp.setScale(3);
    disp.setCursor(37, 2);
    disp.print(dispbuf);
    // disp.update();

    sprintf(dispbuf, "T1:%0.1fC    T2:%0.1fC", sensors.dsT, sensors.bmeT);
    disp.setScale(1);
    disp.setCursor(5, 6);
    disp.print(dispbuf);
    // disp.update();

    sprintf(dispbuf, "H:%02d   U:%003d   P:%003d", sensors.bmeH, sensors.voltage, sensors.bmeP_mmHg);
    disp.setCursor(5, 7);
    disp.print(dispbuf);
    disp.update();
    break;

  case SetZero:
    disp.clear();
    disp.setScale(2);
    disp.setCursor(1, 1);
    disp.print(F(
        " Установка \r\n"
        "   нуля \r\n"
        " подождите..  \r\n"));
    disp.update();

    now = millis();
    while (millis() - now < 5000)
    {
      // тут в течение 5000 миллисекунд вертится код
      // удобно использовать для всяких калибровок
    }
    System.DispMenu = Action;
    disp.clear();
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
  sendATCommand(message + "\r\n" + (String)((char)26), true); // 26
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

  Serial.println(F("!!!!!!!!!!!!!!  DEBUG INFO  !!!!!!!!!!!!!!!!!!"));

  sprintf(message, "DISP:%d | ML %d | T: %02d:%02d ", System.DispState, System.DispMenu, tmrMin, tmrSec);
  Serial.println(message);

  sprintf(message, "TimeRTC: %02d:%02d:%02d", Clock.hour, Clock.minute, Clock.second);
  Serial.println(message);
  sprintf(message, "T_DS:%0.2f *C", sensors.dsT);
  Serial.println(message);
  sprintf(message, "T_BME:%0.2f *C | H_BME:%0d % | P_BHE:%d", sensors.bmeT, (int)sensors.bmeH, (int)sensors.bmeP_mmHg);
  Serial.println(message);
  sprintf(message, "WEIGHT: %0.1fg | W_CAL: %0.2fg  | W_EEP: %f", sensors.kg, sensors.calib);
  Serial.println(message);
  sprintf(message, "BAT: %003d", sensors.voltage);
  Serial.println(message);

  Serial.println(F("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
  Serial.println();
}

// Every 5 seconds  (get data from sensors)
void task0()
{
  GetBatVoltage();
  GetBMEData();
  GetDSData();
  if (ST.Calibration == EEP_DONE)
    GetWeight();
}

// Every 500ms (RTC) and HX711
void task1()
{
  Clock = RTC.getTime();
}

// Display Control
void task2()
{
  // task_counter++;

  if (System.DispState)
  {
    DisplayHandler(System.DispMenu);

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
  else
    disp.setPower(false);

  if DISP_TIME
  {
    System.DispState = false;
    Serial.println("TimeOut: Display - OFF");
    tmrMin = 0;
    tmrSec = 0;
  }
}

// debug
void task3()
{
  Serial.println("Task _ 3");
  ShowDBG();
}