#ifndef _Config_H
#define _Config_H

#include <Arduino.h>


// #include "FileConfig.h"
#include <GyverOS.h>
#include <ArduinoJson.h>
#include "HardwareSerial.h"
#include "SPIFFS.h"
#include <EEPROM.h>
#include <microDS3231.h>
// #include <microDS18B20.h>
// #include "SoftwareSerial.h"
#include <Wire.h>
#include <GyverBME280.h>     

#include <OneWire.h>
#include <DallasTemperature.h>

#include "HX711.h"
#include <EncButton.h>
#include <GyverOLED.h>

#define UARTSpeed 115200
#define MODEMSpeed 9600

#define WiFi_

#define CALL_FAIL 255
#define EEP_DONE 200

#define E_UW 6
#define E_Start 5
#define E_StateCal 4
#define E_Calibr 0


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
const uint8_t logo_32x29[] PROGMEM = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xE7, 0x6F, 0x1F, 0x1F, 0x6F, 0xE7, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x7F, 0x3F, 0x3F, 0x1F, 0x7F, 0xDF, 0x9B, 0x98, 0x98, 0x98, 0x98, 0x9B, 0xDF, 0x7F, 0x1F, 0x3F, 0x3F, 0x7F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0xF0, 0xF0, 0xF0, 0xF8, 0xF0, 0xF0, 0xF1, 0xFD, 0xFC, 0xCC, 0xCC, 0xFC, 0xFC, 0xF1, 0xF0, 0xF0, 0xF8, 0xF0, 0xF0, 0xF0, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xF9, 0xF7, 0xF3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF3, 0xF7, 0xF9, 0xFF, 
};


//========================== ENUMERATION ================================
//=======================================================================
enum menu
{
  Menu = 1,
  Action,
  SetZero,
  Time,
  Calib,
  Notifycation,
  Battery,
  IDLE
};

//=======================================================================

//=========================== GLOBAL CONFIG =============================
struct GlobalConfig
{
  int sn = 0;

  String phone = "+79506045565"; // номер телефона в международном формате
  String firmware = "";          // accepts from setup()
  // System_Information
  String fwdate = "24.02.2024";
  String chipID = "";
  String MacAdr = "";

  String APSSID = "Beekeeper";
  String APPAS = "12345678";

  byte WiFiMode = 0; // Режим работы WiFi
  long WiFiPeriod = 0;
};
extern GlobalConfig Config;
//=======================================================================

//=======================================================================
struct SYTM
{
  bool RelayState = false;
  bool DispState = true;
  uint8_t DispMenu = Action;
  int8_t UserSendTime1 = 9;
  int8_t UserSendTime2 = 20;
};
extern SYTM System;
//=======================================================================

struct SNS
{
  float dsT = 0.0;      // Temperature DS18B20
  float bmeT = 0.0;     // Temperature BME280
  int bmeH = 0.0;     // Humidity   BME280
  float bmeHcal  = 4.2;
  float bmeA = 0.0;     // Altitude   BME280 m
  float bmeP_hPa = 0;   // Pressure   BME280 hPa
  int bmeP_mmHg = 0;  // Pressure   BME280 mmHg
  float calib = 23850; // 0.77
  float units = 0.0;
  float kg = 0.0;
  float g_eep = 2.31;
  float grms = 10.5;
  float g_contain = 0.0;
  uint32_t voltage  = 0;
};
extern SNS sensors;
//=======================================================================

//=======================================================================
struct Flag
{
  uint8_t FirstStart = 0;
  uint8_t Calibration = 0;
};
extern Flag ST;
//============================================================================

//============================================================================
void SystemInit(void);     //  System Initialisation (variables and structure)
void ShowInfoDevice(void); //  Show information or this Device
void GetChipID(void);
void CheckSystemState(void);
void DebugControl(void);
void SystemFactoryReset(void);
//============================================================================
#endif // _Config_H