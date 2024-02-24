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

enum CHANNEL
{
  CH_A = 1,
  CH_B,
  CH_AB
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
  String firmware = ""; // accepts from setup()
  String fwdate = "";
  String chipID = "";
  String MacAdr = "";
  String APSSID = "Beekeeper";
  String APPAS = "CS0120rTra";

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
struct HardwareConfig
{

};
extern HardwareConfig HWConfig;
//=======================================================================

//=======================================================================

//=======================================================================
struct Flag
{
  bool Start : 1;
  bool WiFiEnable: 1;
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