#ifndef LEDSTRIP_HPP
#define LEDSTRIP_HPP

#include <Adafruit_NeoPixel.h>
#include "LedSegment.hpp"



#define NEOPIXELPIN    4
#define NUMLEDS 144

void startLedStripTask();
void LedStripTask(void *parameter);







#endif