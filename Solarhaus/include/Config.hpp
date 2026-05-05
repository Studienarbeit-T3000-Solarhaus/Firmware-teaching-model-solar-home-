#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LedSegment.hpp"

#define NUM_NEOPIXELS 140

// WIFI Configuration
extern const char* ssid;
extern const char* password;

extern Adafruit_NeoPixel Neopixels; 

extern const uint32_t ColorCurrentflow;
extern const uint32_t ColorChargeCaps;
extern const uint32_t ColorDischargeCaps;
extern const uint32_t ColorWhite;

// LED Segment Pointer 

extern LedSegment* AllSolarModulesIndices;
extern LedSegment* SolarModules[4];
extern LedSegment* AllCapacitorsIndices;
extern LedSegment* Capacitors[4];
extern LedSegment* allLoads;
extern LedSegment* AfterBuckBoost; 
extern LedSegment* toOtherLoads;
extern LedSegment* constantLoad;
extern LedSegment* nightLoad;
extern LedSegment* heavyLoad;

// Length and Startindices 

extern const int IndicesAllSolarModules[19];
extern const int* const solarModulesIndices[4];
extern const int IndicesAllCapacitors[2];
extern const int* const capacitorsIndices[4];
extern const int IndicesAllLoads[3];
extern const int IndicesAfterBuckBoost[3];
extern const int IndicesToOtherLoads[4];
extern const int IndicesConstantLoad[2];
extern const int IndicesNightLoad[2];
extern const int IndicesHeavyLoad[2];

extern const uint8_t LengthAllSolarModules;
extern const uint8_t LengthSolarModules[4];
extern const uint8_t LengthAllCapacitors;
extern const uint8_t LengthCapacitors[4];
extern const uint8_t LengthAllLoads;
extern const uint8_t LengthAfterBuckBoost;
extern const uint8_t LengthToOtherLoads;
extern const uint8_t LengthConstantLoad;
extern const uint8_t LengthNightLoad;
extern const uint8_t LengthHeavyLoad;


#endif