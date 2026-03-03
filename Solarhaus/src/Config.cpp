#include "Config.hpp"

// WIFI
const char* ssid = "SolarHaus_WiFi";
const char* password = "";

// Farben definieren
// Achtung: Neopixels.Color() kann hier noch nicht aufgerufen werden, 
// wenn es statisch ist, daher machen wir es oft hardcoded oder nutzen eine Helper-Funktion.
// Aber für den Compiler reicht oft die Zuweisung zur Laufzeit oder einfache Hex-Werte:
// Grün: 0x00FF00, Rot: 0xFF0000, Gelb: 0xFFFF00 (im GRB Format je nach Strip)
const uint32_t ColorCurrentflow = 0xFFFF00; // Gelb
const uint32_t ColorChargeCaps  = 0x00FF00; // Grün
const uint32_t ColorDischargeCaps = 0xFF0000; // Rot

// Speicher für die Arrays reservieren
LedSegment* solarModules[4];
LedSegment* allSolarModules;
LedSegment* allCapacitors;
LedSegment* capacitors[4];
LedSegment* allLoads;
LedSegment* constantLoad;
LedSegment* nightLoad;
LedSegment* heavyLoad;
LedSegment* testSegments[2];

// Werte befüllen
const uint8_t solarModulesLengths[4] = {4, 4, 2, 2}; 
const uint8_t capacitorsLengths[4] = {3, 3, 3, 3}; 
const uint8_t loadLengths[3] = {3, 3, 3};
 

const uint8_t allSolarModulesLength = 8;
const uint8_t allCapacitorsLength = 8; 
const uint8_t allLoadsLength = 5; 

const uint8_t solarModulesStart[4] = {0, 4, 8, 10};
const uint8_t capacitorsStart[4] = {28, 31, 34, 37};
const uint8_t loadStart[3] = {45, 48, 51};


const uint8_t allSolarModulesStart = 12;
const uint8_t allCapacitorsStart = 20;
const uint8_t allLoadsStart = 40;

// 1. Die genauen Indizes als int-Arrays
const int test1Indices[8] = {60, 61, 62, 63, 68, 69, 70, 71};
const int test2Indices[8] = {64, 65, 66, 67, 72, 73, 74, 75};

// 2. Das Array von Zeigern auf die obigen Arrays
const int* const testIndices[2] = {
    test1Indices,
    test2Indices
};

// 3. Nur EINMAL die Längen definieren (am besten über sizeof)
const int testLengths[2] = {
    sizeof(test1Indices) / sizeof(test1Indices[0]),
    sizeof(test2Indices) / sizeof(test2Indices[0])
};