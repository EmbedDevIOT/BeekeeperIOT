#ifndef _Config_H
#define _Config_H

#include <Arduino.h>


// #include "FileConfig.h"
#include <GyverOS.h>
#include <ArduinoJson.h>
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

#define CALL_FAIL 0
#define EEP_DONE 200

#define E_UW 6
#define E_Start 5
#define E_SCal 4
#define E_Cal 0


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
//=======================================================================
enum menu
{
  Menu = 1,
  Action,
  SetZero,
  Time,
  Date,
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

  String phone = "+79524645894"; // номер телефона в международном формате
  String firmware = "";          // accepts from setup()
  // System_Information
  String fwdate = "24.02.2024";
  String chipID = "";
  String MacAdr = "";

  String APSSID = "Beekeeper";
  String APPAS = "12345678";

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
  bool DispState = true;
  uint8_t DispMenu = Action;
  bool RelayState = false;
  int16_t yearSet;
  int16_t monthSet;
  int16_t dateSet;
  int16_t daySet;
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
  float grams = 0.0;
  float g_eeprom = 0.0;
  float grms = 10.5;
  float g_obnul = 0;
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