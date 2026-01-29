#ifndef WEBPORTAL_H
#define WEBPORTAL_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include "CurrentSensor/INA3221Reader.h"


extern Adafruit_MCP23X17 mcp;
extern SemaphoreHandle_t i2cMutex;

void startWebTask();

#endif // WEBPORTAL_H