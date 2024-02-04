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

/*******************************************************************************************************/
// Debug information
void DebugControl()
{
    char message[37];

    Serial.println(F("!!!!!!!!!!!!!!  DEBUG INFO  !!!!!!!!!!!!!!!!!!"));
    sprintf(message, "RTC Time: %02d:%02d:%02d", Clock.hour, Clock.minute, Clock.second);
    Serial.println(message);
    sprintf(message, "RTC Date: %4d.%02d.%02d", Clock.year, Clock.month, Clock.date);
    Serial.println(message);

}

/*******************************************************************************************************/

/*******************************************************************************************************/
void SystemFactoryReset()
{
  Config.TimeZone = 3;
  Config.WiFiMode = AccessPoint;
  Config.APSSID = "ClockStation";
  Config.APPAS = "CS0120rTra";
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
  HWConfig.MinBatLimit = 30;
  HWConfig.PulseFast = 300;
  HWConfig.PulseNormal = 500;
}
/*******************************************************************************************************/
