#ifndef _Config_H
#define _Config_H

#include <Arduino.h>

// #define OS_BENCH

// #include "GyverOS.h"
#include "SystemProcessor.h"
#include "ADC.h"
#include "ClockProcessor.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include "SPIFFS.h"
#include <microDS3231.h>
#include <EncButton.h>
#include "TinyGPSPlus.h"
#include <ArduinoJson.h>
#include "NMEA.h"
#include "ACS712.h"

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

#define LED_GREEN_OFF PCF8574PowerEN(LED_GR, !LOW)
#define LED_GREEN_ON PCF8574PowerEN(LED_GR, !LOW)

#define GPS_S_POINT_1 (FlagState.Start == 1)
#define GPS_S_POINT_2 ((GPSTime.LocalHour == 14) && (GPSTime.Minute == 35) && (GPSTime.Second == 00))

// #define GPS_ONCE_PREP (PrimaryClock.hour == Config.GPSStartHour) && (PrimaryClock.minute == (Config.GPSStartMin - 1))

//=======================================================================
extern MicroDS3231 RTC;
extern DateTime PrimaryClock;
extern ACS712 ACS;
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

enum GPSMODE 
{
  GPS_OFF = 0,
  GPS_ONCE, 
  GPS_ALWAYS
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
  String APSSID = "ClockStation";
  String APPAS = "CS0120rTra";
  // String Ssid = "Keenetic-L3-2.4-prg"; // SSID Wifi network
  // String Password = "QFCxfXMA3";       // Passwords WiFi network
  String Ssid = "AECorp2G";      // SSID Wifi network
  String Password = "Ae19co90$"; // Paswords WiFi network
  char user[5] = "pepa";         //
  char user_pass[7] = "qWe123";
  byte usok = 0;
  int TimeZone = 0;
  byte LedStartHour = 18;
  byte LedStartMinute = 30;
  byte LedFinishHour = 18;
  byte LedFinishMinute = 30;
  byte GPSStartHour = 01;
  byte GPSStartMin = 00;
  byte GPSStartSec = 00;
  byte GPSSynh = 1;
  byte LedOnOFF = 1;    // Флаг старта работы подсветки
  byte LedON = 0;
  int senslim = 1500;   // порог срабатывания чувствительности датчика света
  int i_sens = 0;     // sensetivity current sensor
  int i_cor = 1680;     // error current sensor
  int iLimit = 150;     // Current Protect
  byte IP1 = 192;
  byte IP2 = 168;
  byte IP3 = 1;
  byte IP4 = 31;
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
  byte RSMode = 1;   // Режим работы RS
  byte GPSMode = 1;
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
struct MechanicalClock
{
  byte Hour = 0;
  byte Minute = 0;
  byte Second = 0;
  byte Hold_time = 0;
  byte Polarity = 0;
  byte Start = 0;
  byte ClockST = 2;
  byte Volt = 24;
};
extern MechanicalClock SecondaryClock;
//=======================================================================
struct MechanicalClock2
{
  byte Hour = 0;
  byte Minute = 0;
  byte Second = 0;
  byte Hold_time = 0;
  byte Polarity = 0;
  byte Start = 0;
  byte StopButton = 1;
  byte ClockST = 2;
  byte Volt = 24;
};
extern MechanicalClock2 SecondaryClock2;
//=======================================================================

//=======================================================================
struct Flag
{
  bool Start : 1;
  bool GPSSave : 1;
  bool OnceGPS : 1;
  bool IDLE : 1;
  bool Clock1EN : 1;
  bool GoHome : 1;
  bool GPSEN : 1;
  bool TimState : 1;
  bool LedR : 1;
  bool LedG : 1;
  bool LedB : 1;
  bool SaveFlash : 1;
  bool rs : 1;
  bool in1 : 1;
  bool in2 : 1;
  bool in3 : 1;
  bool in4 : 1;
  bool dcen : 1;
  bool dcvcc : 1;
  bool BackLight : 1;
  bool Debug: 1;
  bool CurDebug: 1;
  bool WiFiEnable: 1;
};
extern Flag FlagState;
//============================================================================

//============================================================================
void SystemInit(void);     //  System Initialisation (variables and structure)
void ShowInfoDevice(void); //  Show information or this Device
void ShowStartInfoORDevice(void);
void GetChipID(void);
String GetMacAdr();
void CheckSystemState(void);
void CheckChargeState(void);
int CheckLightSensor(void);
void DebugControl(void);
void SystemFactoryReset(void);
void SetStateGPS(void);
void ShowFlashSave(void);
//============================================================================
#endif // _Config_H