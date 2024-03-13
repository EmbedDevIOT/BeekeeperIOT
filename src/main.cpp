#include "Config.h"
//=======================================================================

//========================== DEFINITIONS ================================
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5        /* Time ESP32 will go to sleep (in seconds) */

// #define EB_CLICK_TIME 100 // Button timeout
#define DISP_TIME (tmrMin == 10 && tmrSec == 0)
#define ITEMS 5 // Main Menu Items

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
bool st = false; // menu state ()selection

char charPhoneNumber[11];

String currStr = "";
boolean isStringMessage = false;

RTC_DATA_ATTR int bootCount = 0;
//================================ OBJECTs =============================
#define OLED_SOFT_BUFFER_64 // Буфер на стороне МК
GyverOLED<SSD1306_128x64> disp;

// SSD1306 display(0x3c, 21, 22);
// int frameCount = 4;

GyverOS<4> os;
HX711 scale; //
HardwareSerial SIM800(1);
// Serial1 SIM800(TX_PIN, RX_PIN);
MicroDS3231 RTC; // 0x68
GyverBME280 bme; // 0x76
Button btUP(PL_PIN, INPUT_PULLUP);
Button btSET(SET_PIN, INPUT_PULLUP);
Button btDWN(MN_PIN, INPUT_PULLUP);
VirtButton btVirt;

// Dallas Themperature sensor DS18b20
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
/*
Reading Calibration data from EEPROM. Calibration metodic
* Calibration State
* Calibration factor HX711
* First Start Flag
* Container weight.
*/
void EEPROM_Init()
{
  // ADRs: 0 - Calibration 4 - State_Calibration (Done or False) 5 - FirstStart State
  EEPROM.begin(100);

  disp.setScale(2); // масштаб текста (1..4)
  disp.setCursor(13, 3);
  disp.print("Загрузка");
  disp.update();

  Serial.print("EEPROM: CalibST: ");
  ST.Calibration = EEPROM.read(0);
  Serial.print(ST.Calibration);
  Serial.println();

  // Сalibration (First Start)
  now = millis();
  while (millis() - now < 2000)
  {
    btSET.tick();

    if (btSET.press())
    {
      ST.Calibration = CALL_FAIL;
      ST.FirstStart = CALL_FAIL;
      EEPROM.put(0, ST.Calibration); // write state
      EEPROM.put(7, ST.FirstStart);
      EEPROM.commit();

      Serial.println("User Calibration");
    }
  }
  // Если Весы не откалиброваны
  if (ST.Calibration != EEP_DONE)
  {
    bool st = true;
    scale.set_scale();
    Serial.println("1. Tare... remove any weights from the scale...and PRESS to SET button");

    disp.clear();
    disp.setScale(2);
    disp.setCursor(1, 1);
    disp.print(F(
        "Освободите\r\n"
        "платформу\r\n"
        "  > ОК <\r\n"));
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
    disp.setScale(2);
    disp.setCursor(0, 1);
    disp.print(F(
        "Установите\r\n"
        "   1 КГ \r\n"
        "  > ОК < \r\n"));
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
    Serial.println("4.Zero factor: ");
    Serial.print(sensors.calib);
    Serial.println();

    disp.clear();
    disp.setScale(2);
    disp.setCursor(10, 2);
    disp.print(F(
        "Калибровка \r\n"
        " выполнена \r\n"));
    disp.update();

    Serial.println(F("-= Calibration Done =-"));
    ST.Calibration = EEP_DONE;

    EEPROM.put(0, EEP_DONE);      // write state
    EEPROM.put(2, sensors.calib); // write calibration factor

    EEPROM.commit();
  }
  else
  {
    EEPROM.get(2, sensors.calib);
    Serial.printf("EEPROM: Calib: %f", sensors.calib);
    Serial.println();
    scale.set_scale(sensors.calib);
  }

  delay(3000);

  EEPROM.get(7, ST.FirstStart);
  Serial.printf("EEPROM: StartST: %d", ST.FirstStart);
  Serial.println();

  // Первый запуск устройства. и калибровка ОК. Обнуление
  if (ST.FirstStart != EEP_DONE && ST.Calibration == EEP_DONE)
  {
    Serial.println(F("First Starting..Zeroing"));
    scale.set_scale(sensors.calib);

    disp.clear();
    disp.setScale(2);
    disp.setCursor(1, 1);
    disp.print(F(
        "Освободите\r\n"
        "платформу\r\n"
        "  > ОК <\r\n"));
    disp.update();

    while (1)
    {
      btSET.tick();

      if (btSET.click())
      {
        // st = false;
        sensors.units = scale.get_units(10) / 1000;
        sensors.g_contain = sensors.units / -1;

        Serial.printf("Units: %f", sensors.units);
        Serial.println();
        Serial.printf("Container: %f", sensors.g_contain);
        Serial.println();

        Serial.println("Set zero DONE...");

        disp.clear();
        disp.setScale(2); // масштаб текста (1..4)
        disp.setCursor(13, 3);
        disp.print("Сохранено");
        disp.update();
        delay(500);
        disp.clear();

        ST.FirstStart = EEP_DONE;
        EEPROM.put(7, ST.FirstStart);

        EEPROM.put(12, sensors.g_contain);
        EEPROM.commit();

        Serial.println(F("-=First Start Done=-"));
        return;
      }
    }
  }
  else
  {
    EEPROM.get(12, sensors.g_contain);

    Serial.printf("EEPROM: Container: %f", sensors.g_contain);
    Serial.println();
    Serial.println(F("Done.."));
  }
  // Reading Time SMS Notifications
  EEPROM.get(19, Config.UserSendTime1);
  if (Config.UserSendTime1 == -1)
  {
    Config.UserSendTime1 = 9;
  }
  Serial.printf("EEPROM: SMS_1: %02d", Config.UserSendTime1);
  Serial.println();

  EEPROM.get(17, Config.UserSendTime2);
  if (Config.UserSendTime2 == -1)
  {
    Config.UserSendTime2 = 20;
  }
  Serial.printf("EEPROM: SMS_2: %02d", Config.UserSendTime2);
  Serial.println();
  Serial.println(F("EEPROM_INIT_Done.."));
}
//=======================================================================
void StartingInfo()
{
  char msg[32];
  disp.clear(); // очистка

  disp.setScale(2); // масштаб текста (1..4)
  disp.setCursor(10, 3);
  sprintf(msg, "Beekeeper");
  disp.print(msg);
  Serial.println(msg);

  disp.setScale(1);
  disp.setCursor(20, 7);
  sprintf(msg, "firmware:%s", Config.firmware);
  disp.print(msg);
  Serial.println(msg);

  disp.update();
  delay(1000);

  // disp.clear();
  // disp.drawBitmap(9, 16, logo_32x29, 32, 29, BITMAP_NORMAL, BUF_ADD);

  // disp.update();
  // delay(2000);

  disp.clear();
}
//=======================================================================

//=======================       SETUP     =============================
void setup()
{
  // Set User Phone Number
  for (int i = 0; i < 11; i++)
  {
    charPhoneNumber[i] = (char)(Config.phoneNumber[i] + '0');
  }
  Config.phone += charPhoneNumber;
  // Firmware version
  Config.firmware = "0.9.2";
  // UART Init
  Serial.begin(UARTSpeed);
  Serial1.begin(MODEMSpeed);
  // OLED INIT
  Wire.begin();
  disp.init();
  disp.setContrast(255);
  disp.clear();
  Serial.println(F("OLED...Done"));
  // Show starting info (Firmware)
  StartingInfo();
  // RTC INIT
  RTC.begin();
  // RTC battery crash
  if (RTC.lostPower())
  {
    RTC.setTime(COMPILE_TIME);
  }
  Clock = RTC.getTime();
  Serial.println(F("RTC...Done"));

  // set Time (First Start)
  // now = millis();
  // while (millis() - now < 2000)
  // {
  //   btSET.tick();

  //   if (btSET.press())
  //   {
  //     RTC.setTime(COMPILE_TIME);
  //     Serial.println("Установка времени компиляции");
  //   }
  // }

  // HX711 Init
  scale.begin(HX_DT, HX_CLK);
  // EEPROM Init
  EEPROM_Init();
  // BME and DS SENSOR INIT
  bme.begin(0x76);
  Serial.println(F("BME...Done"));
  ds18b20.begin();
  Serial.println(F("DS18b20...Done"));
  delay(20);
  // Battery pin init
  pinMode(BAT, INPUT);

  // SIM800 INIT
  // // delay(15000);
  // delay(2000);
  // SIM800.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  // sendATCommand("AT", true);
  // sendATCommand("AT+CMGDA=\"DEL ALL\"", true);

  // _response = sendATCommand("AT+CMGF=1;&W", true);
  // _response = sendATCommand("AT+IFC=1, 1", true);
  // _response = sendATCommand("AT+CPBS=\"SM\"", true);
  // _response = sendATCommand("AT+CLIP=1", true); // Включаем АОН
  // _response = sendATCommand("AT+CNMI=1,2,2,1,0", true);
  // delay(10);

  disp.clear();

  GetBatVoltage();
  GetBMEData();
  GetDSData();
  GetWeight();

  os.attach(0, task0, 5000);
  os.attach(1, task1, 500);
  os.attach(2, task2, 1000);
  os.attach(3, task3, 1000);
  // os.detach(3);
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

//=========================      M A I N       ===========================
void loop()
{
  os.tick();
  ButtonHandler();
  // IncommingRing();
  // BeekeeperConroller();

  // Task500ms();
  // Task1000ms();
  // Task1MIN();
  //
}
//=======================================================================

/*******************************************************************************************************/
void BeekeeperConroller()
{
}
/*******************************************************************************************************/

/*******************************************************************************************************/
void ButtonHandler()
{
  uint16_t value = 0;

  btSET.tick();
  btUP.tick();
  btDWN.tick();
  btVirt.tick(btUP, btDWN);

  if (btSET.click())
  {
    Serial.println("State: BTN_ SET_ Click");

    if (System.DispMenu == Action)
    {
      System.DispMenu = Menu;
    }

    if (System.DispState == true)
    {
      if (System.DispMenu == Menu && disp_ptr == 0)
      {
        if (!st)
        {
          System.DispMenu = Menu;
          Serial.println("General Menu:");
          st = true;
        }
        else
        {
          System.DispMenu = Time;
          Serial.println("Time Menu:");
          st = false;
        }
      }

      if (System.DispMenu == Menu && disp_ptr == 1)
      {
        System.DispMenu = Calib;
        Serial.println("Calibration Menu:");
      }

      if (System.DispMenu == Menu && disp_ptr == 2)
      {
        System.DispMenu = Notifycation;
        Serial.println("Notifycation menu:");
      }

      if (System.DispMenu == Menu && disp_ptr == 3)
      {
        System.DispMenu = SMS_NUM;
        Serial.println("Battery:");
      }

      if (System.DispMenu == Menu && disp_ptr == 4)
      {
        Serial.println("Exit");

        System.DispMenu = Action;
        disp_ptr = 0;
        st = false;

        os.start(0);
        os.exec(0);
      }
    }
    else // enable display
    {
      disp.setPower(true);
      System.DispMenu = Action;
      System.DispState = true;
    }

    tmrMin = 0;
    tmrSec = 0;
    disp.clear();

    os.exec(2);
  }

  if (btUP.click() || btUP.hold())
  {
    Serial.println("BTN_UP");
    tmrMin = 0;
    tmrSec = 0;
    if (System.DispMenu == Menu)
      disp_ptr = constrain(disp_ptr + 1, 0, ITEMS - 1);

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
      disp_ptr = constrain(disp_ptr - 1, 0, ITEMS - 1);

    Serial.printf("ptr:%d", disp_ptr);
    Serial.println();
  }

  // if (btSET.hasClicks(2))
  // {
  //   Serial.println("Has double cliks");
  //   Serial.println("---------------------");

  //   String sms = "Bec: ";

  //   sms += String(sensors.kg, 1);
  //   sms += " Kg";
  //   sms += "\n";
  //   sms += "B: ";
  //   sms += sensors.voltage;
  //   sms += " %";
  //   sms += "\n";
  //   sms += "T1: ";
  //   sms += String(sensors.dsT, 1);
  //   sms += " *C";
  //   sms += "\n";
  //   sms += "T2: ";
  //   sms += String(sensors.bmeT, 1);
  //   sms += " *C";
  //   sms += "\n";
  //   sms += "H: ";
  //   sms += sensors.bmeH;
  //   sms += " %";
  //   sms += "\n";
  //   sms += "Pr: ";
  //   sms += sensors.bmeP_mmHg;

  //   Serial.println(sms);
  //   Serial.println("---------------------");

  //   sendSMS(Config.phone, sms);
  // }
}
/*******************************************************************************************************/

/*******************************************************************************************************/
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
/*******************************************************************************************************/

/*******************************************************************************************************/
// Get Data from BME Sensor
void GetBMEData()
{
  sensors.bmeT = bme.readTemperature();
  sensors.bmeH = (int)bme.readHumidity() + sensors.bmeHcal;
  sensors.bmeP_hPa = bme.readPressure();
  sensors.bmeP_mmHg = (int)pressureToMmHg(sensors.bmeP_hPa);
}
/*******************************************************************************************************/

/*******************************************************************************************************/
// Get Data from DS18B20 Sensor
void GetDSData()
{
  ds18b20.requestTemperatures();
  sensors.dsT = ds18b20.getTempCByIndex(0);
}
/*******************************************************************************************************/

/*******************************************************************************************************/
// Get Data from HX711
void GetWeight()
{
  scale.power_up();
  sensors.units = scale.get_units(1);
  sensors.kg = sensors.units / 1000;
  sensors.kg = sensors.kg + sensors.g_contain;
  // protect
  sensors.kg = constrain(sensors.kg, 0.0, 200.0);
  scale.power_down();
}
/*******************************************************************************************************/

/*******************************************************************************************************/
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
/*******************************************************************************************************/

/******************************************* MAIN_MENU *************************************************/
void DisplayHandler(uint8_t item)
{

  switch (item)
  {
    char dispbuf[30];

  case Menu:
  {
    os.stop(0);
    disp.clear(); // Очищаем буфер
    disp.home();  // Курсор в левый верхний угол
    disp.setScale(1);
    disp.print // Вывод всех пунктов
        (F(
            "  Время:\r\n"
            "  Калибровка:\r\n"
            "  Оповещения:\r\n"
            // "  Аккумулятор:\r\n"
            "  Номер СМС:\r\n"
            "  Выход:\r\n"));

    printPointer(disp_ptr); // Show pointer
    disp.update();          // Выводим кадр на дисплей
    break;
  }
  case Action:
  {
    // os.restart(0);

    sprintf(dispbuf, "%02d:%02d", Clock.hour, Clock.minute);
    disp.setScale(2);
    disp.setCursor(0, 0);
    disp.print(dispbuf);

    disp.setCursor(85, 0);
    if (sensors.voltage == 0)
    {
      sprintf(dispbuf, "---");
    }
    else
      sprintf(dispbuf, "%3d", sensors.voltage);
    disp.print(dispbuf);

    sprintf(dispbuf, "%0.1f", sensors.kg);
    disp.setScale(3);
    disp.setCursor(40, 2);
    disp.print(dispbuf);

    sprintf(dispbuf, "T1:%0.1fC     T2:%0.1fC", sensors.dsT, sensors.bmeT);
    disp.setScale(1);
    disp.setCursor(0, 6);
    disp.print(dispbuf);

    sprintf(dispbuf, "H:%02d            P:%003d", sensors.bmeH, sensors.bmeP_mmHg);
    disp.setCursor(0, 7);
    disp.print(dispbuf);
    disp.update();
    break;
  }
  case SetZero:
  {
    char msg[50];

    disp.clear();
    disp.setScale(2);
    disp.setCursor(0, 1);
    disp.print(F(
        " Установка \r\n"
        "   нуля \r\n"
        " подождите..  \r\n"));
    disp.update();

    scale.power_up();
    delay(500);
    sensors.units = scale.get_units(10) / 1000;
    sensors.g_contain = sensors.units / -1;
    scale.power_down();

    now = millis();
    while (millis() - now < 5000)
    {
      // user code
    }
    EEPROM.put(12, sensors.g_contain);
    EEPROM.commit();

    sprintf(msg, "WEIGHT: %0.1fg | W_UNIT: %0.4f  | W_EEP: %0.2f", sensors.kg, sensors.g_contain, sensors.g_eep);
    Serial.println(msg);

    System.DispMenu = Action;
    disp.clear();
    break;
  }
  case Time:
  {
    int8_t _hour = RTC.getHours();
    int8_t _min = RTC.getMinutes();
    bool set = false;

    disp.clear();
    disp.setScale(2);
    disp.setCursor(0, 0);
    disp.print(F(" Установка \r\n"
                 "  времени \r\n"));
    disp.setCursor(35, 5);
    sprintf(dispbuf, "%02d:", _hour);
    disp.print(dispbuf);

    disp.invertText(true);
    sprintf(dispbuf, "%02d", _min);
    disp.print(dispbuf);
    disp.update();

    while (!set)
    {
      bool _setH = false;
      bool _setM = true;

      btSET.tick();
      btUP.tick();
      btDWN.tick();

      // Setting Minute
      while (_setM)
      {
        btSET.tick();
        btUP.tick();
        btDWN.tick();

        if (btUP.click())
        {
          disp.clear();
          disp.setScale(2);
          disp.setCursor(0, 0);
          disp.invertText(false);
          disp.print(F(" Установка \r\n"
                       "  времени \r\n"));
          _min++;
          if (_min > 59)
            _min = 0;

          disp.setCursor(35, 5);
          sprintf(dispbuf, "%02d:", _hour);
          disp.print(dispbuf);

          disp.invertText(true);
          sprintf(dispbuf, "%02d", _min);
          disp.print(dispbuf);
          disp.update();
        }

        if (btDWN.click())
        {
          disp.clear();
          disp.setCursor(0, 0);
          disp.setScale(2);
          disp.invertText(false);
          disp.print(F(" Установка \r\n"
                       "  времени \r\n"));
          _min--;
          if (_min < 0)
            _min = 59;

          disp.setCursor(35, 5);
          sprintf(dispbuf, "%02d:", _hour);
          disp.print(dispbuf);

          disp.invertText(true);
          sprintf(dispbuf, "%02d", _min);
          disp.print(dispbuf);
          disp.update();
        }
        // Exit Set MIN and select Hour set
        if (btSET.click())
        {
          _setM = false;
          _setH = true;
          Serial.println(F("Minute set"));

          disp.clear();
          disp.setScale(2);
          disp.setCursor(0, 0);
          disp.invertText(false);
          disp.print(F(" Установка \r\n"
                       "  времени \r\n"));
          disp.setCursor(35, 5);
          disp.invertText(true);
          sprintf(dispbuf, "%02d", _hour);
          disp.print(dispbuf);

          disp.invertText(false);
          sprintf(dispbuf, ":%02d", _min);
          disp.print(dispbuf);
          disp.update();
        }
      }
      //  HOUR
      while (_setH)
      {
        btSET.tick();
        btUP.tick();
        btDWN.tick();

        if (btUP.click())
        {
          disp.clear();
          disp.setCursor(0, 0);
          disp.setScale(2);
          disp.invertText(false);
          disp.print(F(" Установка \r\n"
                       "  времени \r\n"));
          _hour++;
          if (_hour > 23)
            _hour = 0;

          disp.invertText(true);
          disp.setCursor(35, 5);
          sprintf(dispbuf, "%02d", _hour);
          disp.print(dispbuf);

          disp.invertText(false);
          sprintf(dispbuf, ":%02d", _min);
          disp.print(dispbuf);
          disp.update();
        }

        if (btDWN.click())
        {
          disp.clear();
          disp.setCursor(0, 0);
          disp.setScale(2);
          disp.invertText(false);
          disp.print(F(" Установка \r\n"
                       "  времени \r\n"));
          _hour--;
          if (_hour < 0)
            _hour = 23;

          disp.invertText(true);
          disp.setCursor(35, 5);
          sprintf(dispbuf, "%02d", _hour);
          disp.print(dispbuf);

          disp.invertText(false);
          sprintf(dispbuf, ":%02d", _min);
          disp.print(dispbuf);
          disp.update();
        }
        // Exit Set HOUR and SAVE settings
        if (btSET.click())
        {
          _setM = false; // flag set Min (need to exit)
          _setH = false; // flag set Hour(need to exit)
          set = true;    // flag set Time (need to exit)
          Serial.println(F("HOUR set"));
          RTC.setTime(0, _min, _hour, Clock.date, Clock.month, Clock.year);

          st = false;
          System.DispMenu = Action;
          disp_ptr = 0;

          disp.clear();
          disp.invertText(false);
          disp.setScale(2);
          disp.setCursor(13, 3);
          disp.print("Сохранено");
          disp.update();
          delay(500);
          disp.clear();
          // Starting sensors request
          os.start(0);
          os.exec(0);
        }
      }
    }
    break;
  }
  case Calib:
  {
    disp.clear();
    disp.setScale(2); // масштаб текста (1..4)
    disp.setCursor(0, 0);
    disp.print("Калибровка");
    disp.setCursor(17, 5);
    disp.printf("- %0.2f +", sensors.g_contain);
    disp.update();

    while (1)
    {
      btSET.tick();
      btUP.tick();
      btDWN.tick();

      if (btUP.click())
      {
        sensors.g_contain += 0.01;

        disp.clear();
        disp.setScale(2); // масштаб текста (1..4)
        disp.setCursor(0, 0);
        disp.print("Калибровка");
        disp.setCursor(17, 5);
        disp.printf("- %0.2f +", sensors.g_contain);
        disp.update();
      }

      if (btDWN.click())
      {
        sensors.g_contain -= 0.01;

        disp.clear();
        disp.setScale(2); // масштаб текста (1..4)
        disp.setCursor(0, 0);
        disp.print("Калибровка");
        disp.setCursor(17, 5);
        disp.printf("- %0.2f +", sensors.g_contain);
        disp.update();
      }

      // Exit Set CAlibration and SAVE settings
      if (btSET.click())
      {
        EEPROM.put(12, sensors.g_contain);
        EEPROM.commit();

        Serial.println(F("EEPROM: Calibration SAVE"));

        System.DispMenu = Action;
        disp_ptr = 0;
        st = false;

        disp.clear();
        disp.setScale(2); // масштаб текста (1..4)
        disp.setCursor(13, 3);
        disp.print("Сохранено");
        disp.update();
        delay(500);
        disp.clear();
        // Starting sensors request
        os.start(0);
        os.exec(0);
        return;
      }
    }
    break;
  }

  case Notifycation:
  {
    disp.clear();
    disp.invertText(false);
    disp.setScale(2);
    disp.setCursor(13, 0);
    disp.print("Время СМС");
    disp.setCursor(25, 3);
    disp.printf("SMS1:%d", Config.UserSendTime1);
    disp.invertText(true);
    disp.setCursor(25, 5);
    disp.printf("SMS2:%d", Config.UserSendTime2);
    disp.update();

    bool _setSMS1 = false;
    bool _setSMS2 = true;
    // Set SMS_2
    while (_setSMS2)
    {
      btSET.tick();
      btUP.tick();
      btDWN.tick();

      if (btUP.click())
      {
        Config.UserSendTime2 > 23 ? Config.UserSendTime2 = 0 : Config.UserSendTime2++;

        disp.clear();
        disp.invertText(false);
        disp.setScale(2);
        disp.setCursor(13, 0);
        disp.print("Время СМС");
        disp.setCursor(25, 3);
        disp.printf("SMS1:%d", Config.UserSendTime1);
        disp.invertText(true);
        disp.setCursor(25, 5);
        disp.printf("SMS2:%d", Config.UserSendTime2);
        disp.update();
      }

      if (btDWN.click())
      {
        Config.UserSendTime2 < 0 ? Config.UserSendTime2 = 23 : Config.UserSendTime2--;

        disp.clear();
        disp.invertText(false);
        disp.setScale(2);
        disp.setCursor(13, 0);
        disp.print("Время СМС");
        disp.setCursor(25, 3);
        disp.printf("SMS1:%d", Config.UserSendTime1);
        disp.invertText(true);
        disp.setCursor(25, 5);
        disp.printf("SMS2:%d", Config.UserSendTime2);
        disp.update();
      }

      // Exit Set Calibration and SAVE settings
      if (btSET.click())
      {
        EEPROM.put(17, Config.UserSendTime2);
        EEPROM.commit();

        Serial.println(F("EEPROM: SMS_2_MSG SAVE"));

        disp.clear();
        disp.invertText(false);
        disp.setScale(2);
        disp.setCursor(13, 0);
        disp.print("Время СМС");
        disp.setCursor(25, 3);
        disp.invertText(true);
        disp.printf("SMS1:%d", Config.UserSendTime1);
        disp.setCursor(25, 5);
        disp.invertText(false);
        disp.printf("SMS2:%d", Config.UserSendTime2);
        disp.update();

        _setSMS2 = false;
        _setSMS1 = true;
      }
    }
    // Set SMS_1
    while (_setSMS1)
    {
      btSET.tick();
      btUP.tick();
      btDWN.tick();

      if (btUP.click())
      {
        Config.UserSendTime1 > 23 ? Config.UserSendTime1 = 0 : Config.UserSendTime1++;

        disp.clear();
        disp.invertText(false);
        disp.setScale(2);
        disp.setCursor(13, 0);
        disp.print("Время СМС");
        disp.setCursor(25, 3);
        disp.invertText(true);
        disp.printf("SMS1:%d", Config.UserSendTime1);
        disp.setCursor(25, 5);
        disp.invertText(false);
        disp.printf("SMS2:%d", Config.UserSendTime2);
        disp.update();
      }

      if (btDWN.click())
      {
        Config.UserSendTime1 < 0 ? Config.UserSendTime1 = 23 : Config.UserSendTime1--;

        disp.clear();
        disp.invertText(false);
        disp.setScale(2);
        disp.setCursor(13, 0);
        disp.print("Время СМС");
        disp.setCursor(25, 3);
        disp.invertText(true);
        disp.printf("SMS1:%d", Config.UserSendTime1);
        disp.setCursor(25, 5);
        disp.invertText(false);
        disp.printf("SMS2:%d", Config.UserSendTime2);
        disp.update();
      }

      // Exit Set Calibration and SAVE settings
      if (btSET.click())
      {
        EEPROM.put(19, Config.UserSendTime1);
        EEPROM.commit();

        Serial.println(F("EEPROM: SMS_1_MSG SAVE"));

        _setSMS1 = false; // flag to EXIT
        _setSMS2 = false; // flag to EXIT

        System.DispMenu = Action;
        disp_ptr = 0;
        st = false;

        disp.invertText(false);

        disp.clear();

        disp.clear();
        disp.setScale(2);
        disp.setCursor(13, 3);
        disp.print("Сохранено");
        disp.update();
        delay(500);
        disp.clear();
        // Starting sensors request
        os.start(0);
        os.exec(0);
      }
    }
    break;
  }

  case SMS_NUM:
  {
    int currentDigit = 10;

    disp.clear();
    disp.setScale(2); // масштаб текста (1..4)
    disp.setCursor(0, 0);
    disp.print("СМС Номер:");

    disp.setCursor(0, 5);
    for (int i = 0; i < 11; i++)
    {
      if (i == currentDigit)
      {
        disp.invertText(true);
      }
      else
        disp.invertText(false);
      disp.print(Config.phoneNumber[i]);
    }

    disp.update();

    while (currentDigit != -1)
    {
      btSET.tick();
      btUP.tick();
      btDWN.tick();

      if (btUP.click())
      {
        Config.phoneNumber[currentDigit] = (Config.phoneNumber[currentDigit] + 1) % 10;

        disp.clear();
        disp.setScale(2); // масштаб текста (1..4)
        disp.invertText(false);
        disp.setCursor(0, 0);
        disp.print("СМС Номер:");

        disp.setCursor(0, 5);
        for (int i = 0; i < 11; i++)
        {
          (i == currentDigit) ? disp.invertText(true) : disp.invertText(false);
          disp.print(Config.phoneNumber[i]);
        }
        disp.update();
      }

      if (btDWN.click())
      {
        Config.phoneNumber[currentDigit] = (Config.phoneNumber[currentDigit] - 1 + 10) % 10;

        disp.clear();
        disp.setScale(2); // масштаб текста (1..4)
        disp.invertText(false);
        disp.setCursor(0, 0);
        disp.print("СМС Номер:");

        disp.setCursor(0, 5);
        for (int i = 0; i < 11; i++)
        {
          (i == currentDigit) ? disp.invertText(true) : disp.invertText(false);
          disp.print(Config.phoneNumber[i]);
        }
        disp.update();
      }

      // Exit Set CAlibration and SAVE settings
      if (btSET.click())
      {
        currentDigit--;

        Serial.printf("Current Digit: %d", currentDigit);
        Serial.println();

        disp.clear();
        disp.setScale(2); // масштаб текста (1..4)
        disp.invertText(false);
        disp.setCursor(0, 0);
        disp.print("СМС Номер:");

        disp.setCursor(0, 5);
        for (int i = 0; i < 11; i++)
        {
          (i == currentDigit) ? disp.invertText(true) : disp.invertText(false);
          disp.print(Config.phoneNumber[i]);
        }
        disp.update();
      }
    }
    #error
    for (int i = 0; i < 11; i++)
    {
      charPhoneNumber[i] = (char)(Config.phoneNumber[i] + '0');
    }
    Config.phone = charPhoneNumber;

    Serial.printf("EEPROM: SMS Number: %s", Config.phone);
    Serial.println();

    disp.invertText(false);

    System.DispMenu = Action;
    disp_ptr = 0;
    st = false;

    disp.clear();
    disp.setScale(2); // масштаб текста (1..4)
    disp.setCursor(13, 3);
    disp.print("Сохранено");
    disp.update();
    delay(500);
    disp.clear();
    // Starting sensors request
    os.start(0);
    os.exec(0);

    break;
  }

  case Battery:
  {

    break;
  }

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
/*******************************************************************************************************/

/*******************************************************************************************************/
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
/*******************************************************************************************************/

/*******************************************************************************************************/
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
/*******************************************************************************************************/

/*******************************************************************************************************/
void sendSMS(String phone, String message)
{
  sendATCommand("AT+CMGS=\"" + phone + "\"", true);
  sendATCommand(message + "\r\n" + (String)((char)26), true); // 26
}
/*******************************************************************************************************/

/*******************************************************************************************************/
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
/*******************************************************************************************************/

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

// Every 5 seconds  (get data from sensors)
void task0()
{
  Serial.println("Task 5 sec");

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
  ButtonHandler();

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
    disp_ptr = 0;
  }
}

// Task every 1000ms
void task3()
{
  GetBatVoltage();
  ShowDBG();
}

void ShowDBG()
{
  char message[52];

  Serial.println(F("!!!!!!!!!!!!!!  DEBUG INFO  !!!!!!!!!!!!!!!!!!"));

  sprintf(message, "DISP:%d | ML %d | P: %d T: %02d:%02d ", System.DispState, System.DispMenu, disp_ptr, tmrMin, tmrSec);
  Serial.println(message);

  sprintf(message, "TimeRTC: %02d:%02d:%02d", Clock.hour, Clock.minute, Clock.second);
  Serial.println(message);
  sprintf(message, "T_DS:%0.2f *C", sensors.dsT);
  Serial.println(message);
  sprintf(message, "T_BME:%0.2f *C | H_BME:%0d % | P_BHE:%d", sensors.bmeT, (int)sensors.bmeH, (int)sensors.bmeP_mmHg);
  Serial.println(message);
  sprintf(message, "WEIGHT: %0.2fg | W_CAL: %0.5fg  | W_EEP: %0.2f", sensors.kg, sensors.g_contain, sensors.g_eep);
  Serial.println(message);
  sprintf(message, "BAT: %003d", sensors.voltage);
  Serial.println(message);
  sprintf(message, "EEPROM: SMS_1 %02d | SMS_2 %02d", Config.UserSendTime1, Config.UserSendTime2);
  Serial.println(message);

  Serial.println(F("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
  Serial.println();
}
