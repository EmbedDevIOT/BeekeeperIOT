#include "Config.h"
//=======================================================================

/************************ System Initialisation **********************/
void SystemInit(void)
{
  GetChipID();
}
/*******************************************************************************************************/

/***************************** Function Show information or Device *************************************/
void ShowInfoDevice(void)
{
  Serial.println(F("Starting..."));
  Serial.println(F("ClockStation rev.0120"));
  Serial.print(F("SN:"));
  Serial.println(Config.Serial);
  Serial.print(F("fw_date:"));
  Serial.println(Config.fwdate);
  Serial.println(Config.firmware);
  Serial.println(Config.chipID);
  Serial.println(F("by EmbedDev"));
  Serial.println();
}
/*******************************************************************************************************/

void ShowStartInfoORDevice()
{
  // char message[] = {};

  // Serial.println("######################  System Configuration  ######################");
  // Serial.printf("1. WiFi Mode: %d", Config.WiFiMode);
  // Serial.println();
  // Serial.printf("2. Sensor Limit: %000d", Config.senslim);
  // Serial.println();
  // Serial.printf("3. RS485 Mode: %d", Config.RSMode);
  // Serial.println();
  // sprintf(message, "4. DataRTC: %0002d-%02d-%02d", PrimaryClock.year, PrimaryClock.month, PrimaryClock.date);
  // Serial.println(message);
  // sprintf(message, "5. TimeRTC: %02d:%02d:%02d", PrimaryClock.hour, PrimaryClock.minute, PrimaryClock.second);
  // Serial.println(message);
  // sprintf(message, "6. TimeSecondaryClock_1: %02d:%02d:%02d", SecondaryClock.Hour, SecondaryClock.Minute, SecondaryClock.Second);
  // Serial.println(message);
  // sprintf(message, "8. TimeSecondaryClock_2: %02d:%02d:%02d", SecondaryClock2.Hour, SecondaryClock2.Minute, SecondaryClock2.Second);
  // Serial.println(message);
  // Serial.printf("9. Clock1_Start: %d", SecondaryClock.Start);
  // Serial.println();
  // Serial.printf("10. Clock1_Polarity: %d", SecondaryClock.Polarity);
  // Serial.println();
  // sprintf(message, "11. BackLight Start: %02d:%02d", Config.LedStartHour, Config.LedStartMinute);
  // Serial.println(message);
  // sprintf(message, "12. BackLight Stop: %02d:%02d", Config.LedFinishHour, Config.LedFinishMinute);
  // Serial.println(message);
  // Serial.println("#####################################################################");
}
/*******************************************************************************************************/
void GetChipID()
{
  uint32_t chipId = 0;

  for (int i = 0; i < 17; i = i + 8)
  {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  Config.chipID = chipId;
}
/*******************************************************************************************************/

/*******************************************************************************************************/
String GetMacAdr()
{
  Config.MacAdr = WiFi.macAddress(); //
  Serial.print(F("MAC:"));           // временно
  Serial.println(Config.MacAdr);     // временно
  return WiFi.macAddress();
}
/*******************************************************************************************************/

/*******************************************************************************************************/
void CheckSystemState()
{
}
/*******************************************************************************************************/
void CheckChargeState()
{
  if (!digitalRead(BAT_CH))
    Serial.println("BAT: charging");

  if (!digitalRead(BAT_OK))
    Serial.println("BAT: is charged");
}
/*******************************************************************************************************/

/*******************************************************************************************************/
int CheckLightSensor()
{
  int val = 0;

  for (uint8_t i = 0; i < 5; i++)
  {
    val = val + analogRead(SENS_CONT);
  }
  val = val / 5;

  if ((val > 0) && (val <= Config.senslim))
  {
    HWConfig.BrState = true;
  }
  else
  {
// HWConfig.BrState = false;
#warning
    // Serial.println("Sensor disconnect");
  }
  return val;
}
/*******************************************************************************************************/

/*******************************************************************************************************/
// Debug information
void DebugControl()
{
  if (FlagState.Debug)
  {
    char message[37];

    Serial.println(F("!!!!!!!!!!!!!!  DEBUG INFO  !!!!!!!!!!!!!!!!!!"));
    sprintf(message, "RTC Time: %02d:%02d:%02d", PrimaryClock.hour, PrimaryClock.minute, PrimaryClock.second);
    Serial.println(message);
    sprintf(message, "RTC Date: %4d.%02d.%02d", PrimaryClock.year, PrimaryClock.month, PrimaryClock.date);
    Serial.println(message);

    if (Config.GPSMode == GPS_ONCE)
    {
      sprintf(message, "GPS: PWR: %d, MODE: ONCE (to %02d:%02d)", HWConfig.GPSPWR, Config.GPSStartHour, Config.GPSStartMin);
    }
    else
    {
      sprintf(message, "GPS: PWR: %d, MODE: %d", HWConfig.GPSPWR, Config.GPSMode);
    }
    Serial.println(message);

    if (HWConfig.GPSPWR)
    {
      sprintf(message, "GPSLT: %02d:%02d:%02d  TZ: %d", GPSTime.LocalHour, GPSTime.Minute, GPSTime.Second, Config.TimeZone);
      Serial.println(message);
      sprintf(message, "FIX:%d SAT:%0d Age: %d", GPSTime.Fix, GPSTime.Sattelite, GPSTime.Age);
      Serial.println(message);
    }

    sprintf(message, "SC Time: %02d:%02d:%02d", SecondaryClock.Hour, SecondaryClock.Minute, SecondaryClock.Polarity);
    Serial.println(message);
    Serial.printf("BTN Mode %1d", Config.BTNMode);
    Serial.println();
    Serial.printf("Light: %d | SENS: %4d", HWConfig.BrState, HWConfig.LightSens);
    Serial.println();
    sprintf(message, "PWR: %d | Battery: %3d %", HWConfig.PwrState, HWConfig.BTVoltPercent);
    Serial.println(message);

    sprintf(message, "T: %02d", HWConfig.temperature);
    Serial.println(message);

    sprintf(message, "WiFi EN: %2d", FlagState.WiFiEnable);
    Serial.println(message);

    Serial.println(F("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
    Serial.println();
  }
}

/*******************************************************************************************************/
void ShowFlashSave()
{

  bool state = false;

  if (HWConfig.BrState)
    state = true;
  else
    state = false;

  for (uint8_t i = 0; i <= 1; i++)
  {
    PCF8574PowerEN(LIGHT, state);
    delay(400);
    PCF8574PowerEN(LIGHT, !state);
    delay(400);
    // Serial.println("Save");
  }
}
/*******************************************************************************************************/

void SystemFactoryReset()
{
  Config.TimeZone = 3;
  Config.WiFiMode = AccessPoint;
  Config.APSSID = "ClockStation";
  Config.APPAS = "CS0120rTra";
  Config.RSMode = DISABLE;
  Config.GPSMode = GPS_ALWAYS;
  Config.GPSStartHour = 15;
  Config.GPSStartMin = 00;
  Config.GPSStartSec = 00;
  Config.LedStartHour = 17;
  Config.LedStartMinute = 00;
  Config.LedFinishHour = 7;
  Config.LedFinishMinute = 0;
  Config.IP1 = 192;
  Config.IP2 = 168;
  Config.IP3 = 1;
  Config.IP4 = 31;
  Config.GW1 = 192;
  Config.GW2 = 168;
  Config.GW3 = 1;
  Config.GW4 = 1;
  Config.MK1 = 255;
  Config.MK2 = 255;
  Config.MK3 = 255;
  Config.MK4 = 0;
  Config.senslim = 1500;
  HWConfig.MinBatLimit = 30;
  HWConfig.PulseFast = 300;
  HWConfig.PulseNormal = 500;
  SecondaryClock.ClockST = 0;
  SecondaryClock.Hour = 0;
  SecondaryClock.Minute = 0;
  SecondaryClock.Second = 0;
  SecondaryClock.Start = 0;
  SecondaryClock.Volt = 24;
  SecondaryClock2.Volt = 24;
}
/*******************************************************************************************************/
