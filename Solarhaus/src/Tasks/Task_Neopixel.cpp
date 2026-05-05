#include "tasks.hpp"
#include "shared_data.hpp"
#include "Config.hpp"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LedSegment.hpp"
#include "DebugConfig.hpp"

extern Adafruit_NeoPixel Neopixels;

// Visualizes system energy flow on NeoPixel strip: solar input, capacitor state, and load consumption
void Task_Neopixel(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    if (xSemaphoreTake(NeoPixelMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        Neopixels.begin();
        Neopixels.setBrightness(255);
        Neopixels.clear();
        Neopixels.show();
        xSemaphoreGive(NeoPixelMutex);
    }
    
    // Create LED segments for each part of the energy flow diagram
    AllSolarModulesIndices = new LedSegment(&Neopixels, IndicesAllSolarModules, LengthAllSolarModules, ColorCurrentflow);
    for(int i=0; i<4; i++) {
        SolarModules[i] = new LedSegment(&Neopixels, solarModulesIndices[i], LengthSolarModules[i], ColorCurrentflow);
    }
    AllCapacitorsIndices = new LedSegment(&Neopixels, IndicesAllCapacitors, LengthAllCapacitors, ColorCurrentflow);
    for(int i=0; i<4; i++) {
        Capacitors[i] = new LedSegment(&Neopixels, capacitorsIndices[i], LengthCapacitors[i], ColorCurrentflow);
    }
    allLoads = new LedSegment(&Neopixels, IndicesAllLoads, LengthAllLoads, ColorCurrentflow);
    AfterBuckBoost = new LedSegment(&Neopixels, IndicesAfterBuckBoost, LengthAfterBuckBoost, ColorCurrentflow);
    toOtherLoads = new LedSegment(&Neopixels, IndicesToOtherLoads, LengthToOtherLoads, ColorCurrentflow);
    constantLoad = new LedSegment(&Neopixels, IndicesConstantLoad, LengthConstantLoad, ColorCurrentflow);
    nightLoad = new LedSegment(&Neopixels, IndicesNightLoad, LengthNightLoad, ColorCurrentflow);
    nightLoad->setExcludeLast(true);
    nightLoad->setLastPixelColor(ColorWhite);
    nightLoad->setPulse(true);
    heavyLoad = new LedSegment(&Neopixels, IndicesHeavyLoad, LengthHeavyLoad, ColorCurrentflow);
    
    SystemState currentState;

    while(1) {
        if (xSemaphoreTake(NeoPixelMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if(xSemaphoreTake(dataMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                currentState = sysState; 
                xSemaphoreGive(dataMutex);
            } else {
                #ifdef DEBUG
                Serial.println("Failed to acquire data mutex in Neopixel Task");
                #endif
            }
           
            // --- Solar segment animation: speed proportional to measured solar current ---
            float currentSolar = currentState.current_mA[0];
            int activeSolarCount = currentState.solarActiveCount;
            int solarAnimSpeed = 0; 

            if (currentSolar >= 1.0) {
                float clampedCurrent = constrain(currentSolar, 1.0, 40.0);
                solarAnimSpeed = map((long)clampedCurrent, 1, 40, 200, 30);
            }

            for(int i=0; i<4; i++) {
                if (i < activeSolarCount) {
                    SolarModules[i]->setFlow(solarAnimSpeed, false);
                } else {
                    SolarModules[i]->setFlow(0, false);
                    SolarModules[i]->clear();
                }
            }

            // Combined solar bus: faster with more active panels
            if (activeSolarCount > 0 && solarAnimSpeed > 0) {
                int combinedSpeed = solarAnimSpeed / activeSolarCount;
                if (combinedSpeed < 20) combinedSpeed = 20; 
                AllSolarModulesIndices->setFlow(combinedSpeed, false);
            } else {
                AllSolarModulesIndices->setFlow(0, false);
                AllSolarModulesIndices->clear();
            }


            // --- Capacitor segments: flow animation + fill-level bar with color gradient ---
            float currentBat = abs(currentState.current_mA[1]); 
            int activeBatCount = currentState.batteryActiveCount;
            int batAnimSpeed = 0;

            if (currentBat >= 1.0) {
                float clampedBat = constrain(currentBat, 1.0, 100.0); 
                batAnimSpeed = map((long)clampedBat, 1, 100, 400, 60);
            }

            if (activeBatCount > 0 && batAnimSpeed > 0) {
                int combinedBatSpeed = batAnimSpeed; 
                if (combinedBatSpeed < 40) combinedBatSpeed = 40; 
                AllCapacitorsIndices->setFlow(combinedBatSpeed, false);
            } else {
                AllCapacitorsIndices->setFlow(0, false);
                AllCapacitorsIndices->clear();
            }

            // Compute fill percentage and color (red→yellow→green) based on voltage
            float batVoltage = currentState.busVoltage[1]; 
            float minV = 1.2;
            float maxV = 6.1;

            switch(activeBatCount) {
                case 1: maxV = 3.17;  break; 
                case 2: maxV = 4.33;  break; 
                case 3: maxV = 5.23;  break; 
                case 4: maxV = 6.1; break; 
                default: maxV = 0; break; 
            }

            float percentage = 0.0;
            if (maxV > minV) {
                percentage = (batVoltage - minV) / (maxV - minV);
            }
            if (percentage < 0.0) percentage = 0.0;
            if (percentage > 1.0) percentage = 1.0;

            uint8_t r, g;
            uint8_t b = 0;

            if (percentage <= 0.5) {
                r = 255;
                g = (uint8_t)(255.0 * (percentage * 2.0)); 
            } else {
                r = (uint8_t)(255.0 * ((1.0 - percentage) * 2.0));
                g = 255;
            }

            uint32_t batColor = Neopixels.Color(r, g, b);

            for(int i=0; i<4; i++) {
                Capacitors[i]->setFlow(0, false);
                if (i < activeBatCount) {
                    Capacitors[i]->fill(percentage, batColor);
                } else {
                    Capacitors[i]->clear();
                }
            }


            // --- Load segments: animate active consumers, speed scales with total current ---
            float currentLoadTotal = currentState.current_mA[2]; 
            int loadBaseSpeed = 0;

            if (currentLoadTotal >= 5.0) { 
                float clampedLoad = constrain(currentLoadTotal, 5.0, 700.0);
                loadBaseSpeed = map((long)clampedLoad, 5, 700, 200, 30);
            }

            int activeLoadCount = 0;
            if (currentState.constantLoadOn) activeLoadCount++;
            if (currentState.nightLoadOn) activeLoadCount++;
            if (currentState.heavyLoadOn) activeLoadCount++;

            int animSpeed = (loadBaseSpeed > 0) ? loadBaseSpeed : 200;

            if (activeLoadCount > 0) {
                int combinedLoadSpeed = animSpeed / activeLoadCount;
                if (combinedLoadSpeed < 20) combinedLoadSpeed = 20;
                allLoads->setFlow(combinedLoadSpeed, false);
                AfterBuckBoost->setFlow(combinedLoadSpeed, false);
            } else {
                allLoads->setFlow(0, false);
                allLoads->clear();
                AfterBuckBoost->setFlow(0, false);
                AfterBuckBoost->clear();
            }

            if (currentState.nightLoadOn || currentState.heavyLoadOn) {
                toOtherLoads->setFlow(animSpeed, false);
            } else {
                toOtherLoads->setFlow(0, false);
                toOtherLoads->clear();
            }

            if (currentState.constantLoadOn) {
                constantLoad->setFlow(animSpeed, false);
            } else {
                constantLoad->setFlow(0, false);
                constantLoad->clear();
            }

            if (currentState.nightLoadOn) {
                nightLoad->setFlow(animSpeed, false);
            } else {
                nightLoad->setFlow(0, false);
                nightLoad->clear();
            }

            if (currentState.heavyLoadOn) {
                heavyLoad->setFlow(animSpeed, false);
            } else {
                heavyLoad->setFlow(0, false);
                heavyLoad->clear();
            }


            // --- Commit all segment animations to the strip ---
            for(int i=0; i<4; i++) SolarModules[i]->update();
            AllSolarModulesIndices->update();
            AllCapacitorsIndices->update();
            allLoads->update();
            AfterBuckBoost->update();
            toOtherLoads->update();
            constantLoad->update();
            nightLoad->update();
            heavyLoad->update();

            Neopixels.show();
            xSemaphoreGive(NeoPixelMutex);
        }

        vTaskDelayUntil(&xLastWakeTime, PERIOD_NEOPIXEL_TASK);
    }
}