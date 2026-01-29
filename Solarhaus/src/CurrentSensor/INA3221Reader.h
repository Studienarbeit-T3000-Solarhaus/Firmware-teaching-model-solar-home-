#ifndef INA3221READER_H
#define INA3221READER_H

#include <Arduino.h>
#include <Adafruit_INA3221.h>

// Globales Objekt, damit auch der Webserver darauf zugreifen kann
extern Adafruit_INA3221 ina3221;
extern SemaphoreHandle_t i2cMutex;

void startINA3221Task();

#endif