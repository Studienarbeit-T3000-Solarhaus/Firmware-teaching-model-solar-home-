#ifndef LEDSTRIP_HPP
#define LEDSTRIP_HPP

#include <Adafruit_NeoPixel.h>
#include "LedSegment.hpp"
#include "Pins.h"
#include "CurrentSensor/INA3221Reader.h" 



#define NUMLEDS 144

void startLedStripTask();
void LedStripTask(void *parameter);







#endif