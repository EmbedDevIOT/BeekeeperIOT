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
  int Serial = 0;
  String firmware = ""; // accepts from setup()
  String fwdate = "";
  String chipID = "";
  String MacAdr = "";
  String NTPServer = "pool.ntp.org";
  String APSSID = "Beekeeper";
  String APPAS = "CS0120rTra";
  // String Ssid = "Keenetic-L3-2.4-prg"; // SSID Wifi network
  // String Password = "QFCxfXMA3";       // Passwords WiFi network
  String Ssid = "AECorp2G";      // SSID Wifi network
  String Password = "Ae19co90$"; // Paswords WiFi network
  int TimeZone = 0;
  byte IP1 = 192;
  byte IP2 = 168;
  byte IP3 = 4;
  byte IP4 = 1;
  byte GW1 = 192;
  byte GW2 = 168;
  byte GW3 = 1;
  byte GW4 = 1;
  byte MK1 = 255;
  byte MK2 = 255;
  byte MK3 = 255;
  byte MK4 = 0;
  byte WiFiMode = 0; // Режим работы WiFi
  long WiFiPeriod = 0; 
  byte BTNMode = 3;
};
extern GlobalConfig Config;
//=======================================================================

//=======================================================================
struct HardwareConfig
{
  String BTVoltage = ""; // Battery voltage in voltage
  int BTVoltPercent = 0; // Battery voltage in percent
  int MinBatLimit = 40;  // Minimum Battery Voltage
  int LightSens = 0;
  float Current = 0.0;
  int temperature = 0;
  byte BrState = 0;
  byte PwrState = 1; /* !PwrState! 0 - Battery | 1 - PowerSypply | 2 - USB connect  */
  byte PulseH = 0;
  int PulseNormal = 500;
  int PulseFast = 300;
  byte GPSPWR = 1;
  byte DrIN1 = 0;
  byte DrIN2 = 0;
  byte DrIN3 = 0;
  byte DrIN4 = 0;
  byte DrEN1 = 0;
  byte DrEN2 = 0;
  byte DCEnable = 0;
  byte DCVoltage = 0;
  uint8_t TCLpulse = 5; // 5 = 500ms (5 * 100)
  uint8_t ERRORcnt = 0;
};
extern HardwareConfig HWConfig;
//=======================================================================

//=======================================================================
struct GPS
{
  uint32_t Age = 0;
  byte Sattelite = 0;
  byte SatValid = 0;
  byte Fix = 0;
  byte TimeZone = 0;
  byte Hour = 0;
  byte LocalHour = 0;
  byte Minute = 0;
  byte Second = 0;
};
extern GPS GPSTime;
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