#ifndef _Config_H
#define _Config_H

#include <Arduino.h>

#include <WiFi.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"
#include <microDS3231.h>
#include <microDS18B20.h>
#include "SoftwareSerial.h"
#include <Wire.h>
#include "Adafruit_BME280.h"
#include <Adafruit_Sensor.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "HX711.h"
#include <EncButton.h>
#include <GyverOLED.h>

#define UARTSpeed 115200

#define WiFi_

#define WiFiTimeON 15
#define Client 0
#define AccessPoint 1
#define WorkNET

#define DEBUG
// #define I2C_SCAN

#define DISABLE 0
#define ENABLE 1

#define BAT_MIN 30

//=======================================================================
extern MicroDS3231 RTC;
extern DateTime Clock;
//=======================================================================

//========================== ENUMERATION ================================
enum OS_T
{
  T_10MS = 0,
  T_100MS,
  T_300MS,
  T_500MS,
  T_1000MS
};

enum ClockST
{
  STOP = 0,
  START,
  HOME
};

enum Clicks
{
  ONE = 1,
  TWO,
  THREE,
};
//=======================================================================

//=========================== GLOBAL CONFIG =============================
struct GlobalConfig
{
  int sn = 0;

  String phone = "+79524645894"; // номер телефона в международном формате
  String firmware = "";          // accepts from setup()
  // System_Information
  String fwdate = "24.02.2024";
  String chipID = "";
  String MacAdr = "";

  String APSSID = "Beekeeper";
  String APPAS = "12345678";
  String Ssid = "AECorp2G";      // SSID Wifi network
  String Password = "Ae19co90$"; // Paswords WiFi network

  int TimeZone = 0;
  byte WiFiMode = 0; // Режим работы WiFi
  long WiFiPeriod = 0;
  byte BTNMode = 3;
};
extern GlobalConfig Config;
//=======================================================================

//=======================================================================
struct SYTM
{
  int16_t year = 0;
  int16_t month = 0;
  int16_t date = 0;
  int16_t day;
  int16_t hour;
  int16_t min;
  int16_t yearSet;
  int16_t monthSet;
  int16_t dateSet;
  int16_t daySet;
  int16_t daySet;
};
extern SYTM System;
//=======================================================================

struct SNS
{
  float dsT = 0.0;  // Temperature DS18B20
  float bmeT = 0.0; // Temperature BME280
  float bmeH = 0.0; // Humidity   BME280
  int bmeP = 0;     // Pressure   BME280
  float calibr_factor = -0.77;
  float units = 0.0;
  float grams = 0.0;
  float g_eeprom = 0.0;
  float grms;
  float g_obnul = 0;
};
extern SNS sensors;
//=======================================================================

//=======================================================================
struct Flag
{
  bool Start : 1;
  bool WiFiEnable : 1;
};
extern Flag FlagState;
//============================================================================

//============================================================================
void SystemInit(void);     //  System Initialisation (variables and structure)
void ShowInfoDevice(void); //  Show information or this Device
void GetChipID(void);
String GetMacAdr();
void CheckSystemState(void);
void DebugControl(void);
void SystemFactoryReset(void);
//============================================================================
#endif // _Config_H