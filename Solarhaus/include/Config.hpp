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

extern LedSegment* AllSolarModulesIndices;
extern LedSegment* SolarModules[4];
extern LedSegment* AllCapacitorsIndices;
extern LedSegment* Capacitors[4];
extern LedSegment* allLoads;
extern LedSegment* AfterBuckBoost; 

// --- Längen und Startpositionen ---


extern const int IndicesAllSolarModules[19];
extern const int* const solarModulesIndices[4];
extern const int IndicesAllCapacitors[2];
extern const int* const capacitorsIndices[4];
extern const int IndicesAllLoads[3];
extern const int IndicesAfterBuckBoost[7];


extern const uint8_t LengthAllSolarModules;
extern const uint8_t LengthSolarModules[4];
extern const uint8_t LengthAllCapacitors;
extern const uint8_t LengthCapacitors[4];
extern const uint8_t LengthAllLoads;
extern const uint8_t LengthAfterBuckBoost;

#endif