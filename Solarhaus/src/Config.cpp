#include "Config.hpp"
#include "DebugConfig.hpp"

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
LedSegment* AllSolarModulesIndices;
LedSegment* SolarModules[4];
LedSegment* AllCapacitorsIndices;
LedSegment* Capacitors[4];
LedSegment* allLoads;
LedSegment* AfterBuckBoost;
LedSegment* toOtherLoads;
LedSegment* constantLoad;
LedSegment* nightLoad;
LedSegment* heavyLoad;

const int solarMod1[3] = {20, 21, 22};
const int solarMod2[3] = {32, 33, 34};
const int solarMod3[6] = {31, 30, 29, 19, 18, 17};
const int solarMod4[6] = {28, 27, 26, 25, 24, 23};
// Array von Zeigern auf die Module
const int* const solarModulesIndices[4] = {solarMod1, solarMod2, solarMod3, solarMod4};


const int IndicesAllSolarModules[19] = {16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 35, 36};
const int IndicesAllCapacitors[2] = {37, 38};

const int capacitor1[3] = {39, 40, 41};
const int capacitor2[3] = {42, 43, 44};
const int capacitor3[3] = {45, 46, 47};
const int capacitor4[3] = {48, 49, 50};
const int* const capacitorsIndices[4] = {capacitor1, capacitor2, capacitor3, capacitor4};

const int IndicesAllLoads[3] = {51, 52, 53};

const int IndicesAfterBuckBoost[3] = {54, 55, 56};
const int IndicesToOtherLoads[4] = {59, 60, 61, 62};
const int IndicesConstantLoad[2] = {57, 58};
const int IndicesNightLoad[2] = {65, 66};
const int IndicesHeavyLoad[2] = {63, 64};


const uint8_t LengthAllSolarModules = sizeof(IndicesAllSolarModules) / sizeof(IndicesAllSolarModules[0]);
const uint8_t LengthSolarModules[4] = {sizeof(solarMod1), sizeof(solarMod2), sizeof(solarMod3), sizeof(solarMod4)};
const uint8_t LengthAllCapacitors = sizeof(IndicesAllCapacitors) / sizeof(IndicesAllCapacitors[0]);
const uint8_t LengthCapacitors[4] = {sizeof(capacitor1), sizeof(capacitor2), sizeof(capacitor3), sizeof(capacitor4)};
const uint8_t LengthAllLoads = sizeof(IndicesAllLoads) / sizeof(IndicesAllLoads[0]);
const uint8_t LengthAfterBuckBoost = sizeof(IndicesAfterBuckBoost) / sizeof(IndicesAfterBuckBoost[0]);
const uint8_t LengthToOtherLoads = sizeof(IndicesToOtherLoads) / sizeof(IndicesToOtherLoads[0]);
const uint8_t LengthConstantLoad = sizeof(IndicesConstantLoad) / sizeof(IndicesConstantLoad[0]);
const uint8_t LengthNightLoad = sizeof(IndicesNightLoad) / sizeof(IndicesNightLoad[0]);
const uint8_t LengthHeavyLoad = sizeof(IndicesHeavyLoad) / sizeof(IndicesHeavyLoad[0]);