#ifndef FileConfig_H
#define FileConfig_H

#include "Config.h"
#include <ArduinoJson.h>
#include "SPIFFS.h"

void LoadConfig();
void SaveConfig();

#endif