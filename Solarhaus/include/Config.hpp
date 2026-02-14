#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LedSegment.hpp"

#define NUM_NEOPIXELS 140

// WIFI Configuration
extern const char* ssid;
extern const char* password;

// --- Neopixel Objekt (Global verfügbar machen) ---
extern Adafruit_NeoPixel Neopixels; 

// --- Farben (extern machen) ---
extern const uint32_t ColorCurrentflow;
extern const uint32_t ColorChargeCaps;
extern const uint32_t ColorDischargeCaps;

// --- LED Segmente Zeiger ---
extern LedSegment* solarModules[4];
extern LedSegment* allSolarModules;
extern LedSegment* allCapacitors;
extern LedSegment* capacitors[4];
extern LedSegment* allLoads;
extern LedSegment* constantLoad;
extern LedSegment* nightLoad;
extern LedSegment* heavyLoad;

// --- Längen und Startpositionen ---
extern const uint8_t solarModulesLengths[4];
extern const uint8_t capacitorsLengths[4];
extern const uint8_t loadLengths[3];

extern const uint8_t allSolarModulesLength;
extern const uint8_t allCapacitorsLength;
extern const uint8_t allLoadsLength;

extern const uint8_t solarModulesStart[4];
extern const uint8_t capacitorsStart[4];
extern const uint8_t loadStart[3];

extern const uint8_t allSolarModulesStart;
extern const uint8_t allCapacitorsStart;
extern const uint8_t allLoadsStart;

#endif