/*
  Время и дата устанавливаются атвоматически при загрузке прошивки (такие как на компьютере)
  Как настроить время на часах. У нас есть возможность автоматически установить время на время загрузки прошивки, поэтому:
  - Ставим настройку RESET_CLOCK на 1
  - Прошиваемся
  - Сразу ставим RESET_CLOCK 0
  - И прошиваемся ещё раз
  - Всё
 Не забудь поставить 0 и прошить ещё раз!
*/
#define RESET_CLOCK  0
#define BUTTON_1 3
#define BUTTON_2 4
// номер телефона в международном формате
String phone = "";
// настройки таймеров
// таймер 1 час отправки смс
#define H1_ON        9
// таймер 2 час отправки смс
#define H2_ON        20

// библиотеки
#include <DS3231.h>
#include <Wire.h>
#include <OneWire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "HX711.h"
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include "sav_button.h"

// пины
// кнопкa Выбор
#define SET_PIN       2
// кнопкa Плюс
#define PL_PIN        3
// кнопкa Минус
#define MN_PIN        4
// датчик ds18b20
#define DS_PIN        5
// tx
#define TX_PIN        7
// rx
#define RX_PIN        8

SButton bt_set(SET_PIN,  50, 2000, 0, 0);
SButton bt_pl(PL_PIN,  50, 0, 500, 100);
SButton bt_mn(MN_PIN,  50, 0, 500, 100);
HX711 scale(A1, A0); // DT, CLK
LiquidCrystal_I2C lcd(0x27,16,2);
SoftwareSerial SIM800(TX_PIN, RX_PIN);
OneWire ds(DS_PIN);
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;

// часы
DS3231 Clock;
bool h12 = false;
bool PM;
bool Century = false;
int16_t year, month, date, dow, hrs, mins, secs;
int16_t yearSet, monthSet, dateSet, dowSet, hourSet, minSet, secSet;

float tempDs = 0.0;
float bmeTemp;
float bmeHum;
int   bmePres;

uint32_t ms, msSet;
bool bl = false;
bool set = false;
int m = 0;

String currStr = "";
boolean isStringMessage = false;
String _response = "";

float calibr_factor = -0.77;
float units = 0.0;
float grams = 0.0;
float g_eeprom = 0.0;
float grms;
float g_obnul = 0;
byte gradus[8] = {
  0b01000,
  0b10100,
  0b01000,
  0b00011,
  0b00100,
  0b00100,
  0b00011,
  0b00000
};
byte G[8] = {
  0b00000,
  0b00000,
  0b11110,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b00000
};
byte R[8] = {
  0b00000,
  0b00000,
  0b11100,
  0b10010,
  0b11100,
  0b10000,
  0b10000,
  0b00000
};

String waitResponse()
{
  String _resp = "";
  long _timeout = millis() + 10000;
  while (!SIM800.available() && millis() < _timeout) {};
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

void GetClock()
{
  hrs = Clock.getHour(h12, PM);
  mins = Clock.getMinute();
  year = Clock.getYear();
  month = Clock.getMonth(Century);
  date = Clock.getDate();
}

void GetBme()
{
  bme.takeForcedMeasurement();
  bmeTemp = bme.readTemperature();
  bmeHum = bme.readHumidity();
  bmePres = (float)bme.readPressure() * 0.00750062;
}

void GetDs()
{
  static uint32_t msT = 0;
  byte data[2];

  ds.reset();
  ds.write(0xCC);
  ds.write(0x44);

  if (ms - msT > 1000)
  {
    ds.reset();
    ds.write(0xCC);
    ds.write(0xBE);
    data[0] = ds.read();
    data[1] = ds.read();
    tempDs = ((data[1] << 8) | data[0]) * 0.0625;

    msT = ms;
  }
}

void GetW()
{  
  scale.set_scale(calibr_factor);
  units = scale.get_units(), 10;
  grams = (units * 0.035274);
}

void GetData()
{
  static uint32_t msD = 0;
  static uint32_t msA = 0;

  if (ms - msD > 500)
  {
    GetW();
    GetClock();
    GetDs();
    GetBme();
    bl = !bl;
    msD = ms;
  }
  if (ms - msA > 5000)
  {
    msA = ms;
    Serial.println("Zapros Signala");
    SIM800.println("AT+CSQ");
  }
}

void Disp()
{
  lcd.setCursor(0, 0);
  if (set && !bl && m == 0)
    lcd.print("  ");
  else
  {
    if (hrs < 10)
      lcd.print("0");
    lcd.print(hrs);
  }

  if (bl)
    lcd.print(":");
  else
    lcd.print(" ");

  if (set && !bl && m == 1)
    lcd.print("  ");
  else
  {
    if (mins < 10)
      lcd.print("0");
    lcd.print(mins);
  }
  lcd.setCursor(6, 0);
  lcd.print("B ");
  if (set && !bl && m == 2)
    lcd.print("     ");
  else
  {
    if (grms < 10.0)
      lcd.print("  ");
    if (grms >= 10.0 && grms < 100.0)
      lcd.print(" ");
    lcd.print(grms, 1);
  }
  lcd.print(" K\1");

  lcd.setCursor(0, 1);
  if (tempDs < 10.0)
    lcd.print(" ");
  lcd.print(tempDs, 0);
  lcd.print("\3");

  lcd.setCursor(4, 1);
  if (bmeTemp < 10.0)
    lcd.print(" ");
  lcd.print(bmeTemp, 0);
  lcd.print("\3");

  lcd.setCursor(8, 1);
  if (bmeHum < 10.0)
    lcd.print(" ");
  lcd.print(bmeHum, 0);
  lcd.print("%");

  lcd.setCursor(12, 1);
  lcd.print(bmePres);
}

void Menu()
{
  switch (bt_set.Loop())
  {
    case SB_CLICK:
    if (set)
    {
      m++;
      if (m > 2)
      {
        set = false;
        m = 0;
      }
      msSet = ms;
    }
    break;
    case SB_LONG_CLICK:
    set = !set;
    msSet = ms;
    break;
  }

  switch (bt_pl.Loop())
  {
    case SB_CLICK:
    if (set)
    {
      msSet = ms;
      switch (m)
      {
        case 0:
        hourSet = hrs;
        hourSet++;
        if (hourSet > 23)
          hourSet = 0;
        Clock.setHour(hourSet);
        break;
        case 1:
        minSet = mins;
        minSet++;
        if (minSet > 59)
          minSet = 0;
        Clock.setMinute(minSet);
        break;
        case 2:
        calibr_factor += 0.01;
        EEPROM.put(0, calibr_factor);
        delay(5);
        break;
      }
    }
    break;
    case SB_AUTO_CLICK:
    if (set)
    {
      msSet = ms;
      switch (m)
      {
        case 0:
        hourSet = hrs;
        hourSet++;
        if (hourSet > 23)
          hourSet = 0;
        Clock.setHour(hourSet);
        break;
        case 1:
        minSet = mins;
        minSet++;
        if (minSet > 59)
          minSet = 0;
        Clock.setMinute(minSet);
        break;
        case 2:
        calibr_factor += 0.1;
        EEPROM.put(0, calibr_factor);
        delay(5);
        break;
      }
    }
    break;
  }

  switch (bt_mn.Loop())
  {
    case SB_CLICK:
    if (set)
    {
      msSet = ms;
      switch (m)
      {
        case 0:
        hourSet = hrs;
        hourSet--;
        if (hourSet < 0)
          hourSet = 23;
        Clock.setHour(hourSet);
        break;
        case 1:
        minSet = mins;
        minSet--;
        if (minSet < 0)
          minSet = 59;
        Clock.setMinute(minSet);
        break;
        case 2:
        calibr_factor -= 0.01;
        EEPROM.put(0, calibr_factor);
        delay(5);
        break;
      }
    }
    break;
    case SB_AUTO_CLICK:
    if (set)
    {
      msSet = ms;
      switch (m)
      {
        case 0:
        hourSet = hrs;
        hourSet--;
        if (hourSet < 0)
          hourSet = 23;
        Clock.setHour(hourSet);
        break;
        case 1:
        minSet = mins;
        minSet--;
        if (minSet < 0)
          minSet = 59;
        Clock.setMinute(minSet);
        break;
        case 2:
        calibr_factor -= 0.1;
        EEPROM.put(0, calibr_factor);
        delay(5);
        break;
      }
    }
    break;
  }

  if (set)
  {
    if (ms - msSet > 20000)
    {
      set = false;
      m = 0;
    }
  }
}

void Timer()
{
  static bool smsSt = false;
  int now = hrs * 60;
  int On1 = H1_ON * 60;
  int Of1 = (H1_ON+1) * 60;
  int On2 = H2_ON * 60;
  int Of2 = (H2_ON+1) * 60;

  if (((On1 < Of1) && ((On1 <= now) && (now < Of1))) || ((On1 > Of1) && ((now < Of1) || (On1 <= now))) || ((On2 < Of2) && ((On2 <= now) && (now < Of2))) || ((On2 > Of2) && ((now < Of2) || (On2 <= now))))
  {
    if (!smsSt)
    {
      String smska = "Bec ";
      smska += String(grms, 1);
      smska += " Kg";
      smska += "\n";
      smska += "T1 ";
      smska += String(tempDs, 1);
      smska += "\n";
      smska += "T2 ";
      smska += String(bmeTemp, 1);
      smska += "\n";
      smska += "H ";
      smska += String(bmeHum, 1);
      smska += "\n";
      smska += "Pr ";
      smska += String(bmePres);

      Serial.println(smska);
      sendSMS(phone, smska); // смс
      delay(2000);
      smsSt = true;
    }
  }
  else
  {
    smsSt = false;
  }
}

void setup()
{
   EEPROM.get(50, g_eeprom);
  pinMode(BUTTON_1,INPUT_PULLUP);
  pinMode(BUTTON_2,INPUT_PULLUP);
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.createChar(1, G);
  lcd.createChar(2, R);
  lcd.createChar(3, gradus);
  delay(1000);
  lcd.clear();
  delay(20);

  bt_set.begin();
  bt_pl.begin();
  bt_mn.begin();
  delay(20);

  Wire.begin();
  delay(20);

  bme.begin(&Wire);
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1,
                  Adafruit_BME280::SAMPLING_X1,
                  Adafruit_BME280::SAMPLING_X1,
                  Adafruit_BME280::FILTER_OFF   );

  bme.takeForcedMeasurement();
  uint32_t Pressure = bme.readPressure();
  delay(20);

  if (EEPROM.read(100) != 200)
  {
    EEPROM.put(0, calibr_factor);
    EEPROM.write(100, 200);
  }
  EEPROM.get(0, calibr_factor);

  scale.set_scale();
  scale.tare();
  long zero_factor = scale.read_average();

  Serial.println("Start!");
  delay(15000);
  SIM800.begin(9600);
  Serial.println("v1.2!");
  sendATCommand("AT", true);
  sendATCommand("AT+CMGDA=\"DEL ALL\"", true);
  _response = sendATCommand("AT+CMGF=1;&W", true);
  _response = sendATCommand("AT+IFC=1, 1", true);
  _response = sendATCommand("AT+CPBS=\"SM\"", true);
  _response = sendATCommand("AT+CLIP=1", true); // Включаем АОН
  _response = sendATCommand("AT+CNMI=1,2,2,1,0", true);
  delay(10);
  int b = EEPROM.read(62);
  if(b != 200)
  {
    EEPROM.put(50, 0.0);
    EEPROM.write(62,200);
  }

}

void loop()
{
  grms = float(grams / 1000)+g_eeprom-g_obnul;
  EEPROM.put(50, grms);
  if(!digitalRead(BUTTON_1)&&!digitalRead(BUTTON_2))
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Zeroing");
    lcd.setCursor(0,1);
    lcd.print("Please wait");
    for(int a = 0; a < 500; a++)for(int i = 0; i < 32000; i++)asm("NOP");;
    lcd.clear();
    g_obnul += grms;
  }
  
  ms = millis();
  GetData();
  Disp();
  Menu();
  Timer();
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
         
        String smska = "Bec ";
        smska += String(grms, 1);
        smska += " Kg";
        smska += "\n";
        smska += "T1 ";
        smska += String(tempDs, 1);
        smska += "\n";
        smska += "T2 ";
        smska += String(bmeTemp, 1);
        smska += "\n";
        smska += "H ";
        smska += String(bmeHum, 1);
        smska += "\n";
        smska += "Pr ";
        smska += String(bmePres);

        Serial.println(smska);
        sendSMS(innerPhone, smska); // смс
        delay(2000);
      }
    }

  }
}
