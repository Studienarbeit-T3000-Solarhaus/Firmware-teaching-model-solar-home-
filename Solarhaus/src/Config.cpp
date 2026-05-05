#include "Config.hpp"
#include "DebugConfig.hpp"

// WiFi Access Point credentials (open network)
const char* ssid = "SolarHaus_WiFi";
const char* password = "";

// LED animation colors (raw RGB hex values)
const uint32_t ColorCurrentflow = 0xFFFF00;
const uint32_t ColorChargeCaps  = 0x00FF00;
const uint32_t ColorDischargeCaps = 0xFF0000;
const uint32_t ColorWhite = 0xFFFFFF;

// LED segment pointers (allocated at runtime in Task_Neopixel)
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

// NeoPixel index mappings: each array defines the physical LED order for a segment
const int solarMod1[3] = {20, 21, 22};
const int solarMod2[3] = {32, 33, 34};
const int solarMod3[6] = {31, 30, 29, 19, 18, 17};
const int solarMod4[6] = {28, 27, 26, 25, 24, 23};
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

// Segment lengths (auto-calculated from index arrays)
const uint8_t LengthAllSolarModules = sizeof(IndicesAllSolarModules) / sizeof(IndicesAllSolarModules[0]);
const uint8_t LengthSolarModules[4] = {
    sizeof(solarMod1) / sizeof(solarMod1[0]), 
    sizeof(solarMod2) / sizeof(solarMod2[0]), 
    sizeof(solarMod3) / sizeof(solarMod3[0]), 
    sizeof(solarMod4) / sizeof(solarMod4[0])
};

const uint8_t LengthCapacitors[4] = {
    sizeof(capacitor1) / sizeof(capacitor1[0]), 
    sizeof(capacitor2) / sizeof(capacitor2[0]), 
    sizeof(capacitor3) / sizeof(capacitor3[0]), 
    sizeof(capacitor4) / sizeof(capacitor4[0])
};
const uint8_t LengthAllCapacitors = sizeof(IndicesAllCapacitors) / sizeof(IndicesAllCapacitors[0]);

const uint8_t LengthAllLoads = sizeof(IndicesAllLoads) / sizeof(IndicesAllLoads[0]);
const uint8_t LengthAfterBuckBoost = sizeof(IndicesAfterBuckBoost) / sizeof(IndicesAfterBuckBoost[0]);
const uint8_t LengthToOtherLoads = sizeof(IndicesToOtherLoads) / sizeof(IndicesToOtherLoads[0]);
const uint8_t LengthConstantLoad = sizeof(IndicesConstantLoad) / sizeof(IndicesConstantLoad[0]);
const uint8_t LengthNightLoad = sizeof(IndicesNightLoad) / sizeof(IndicesNightLoad[0]);
const uint8_t LengthHeavyLoad = sizeof(IndicesHeavyLoad) / sizeof(IndicesHeavyLoad[0]);