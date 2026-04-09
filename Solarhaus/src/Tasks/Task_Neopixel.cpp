#include "tasks.hpp"
#include "shared_data.hpp"
#include "Config.hpp"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LedSegment.hpp"
#include "DebugConfig.hpp"

extern Adafruit_NeoPixel Neopixels;

void Task_Neopixel(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    if (xSemaphoreTake(NeoPixelMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        Neopixels.begin();
        Neopixels.setBrightness(255); // Global auf Max, Segmente dimmen intern
        Neopixels.clear();
        Neopixels.show();
        xSemaphoreGive(NeoPixelMutex);
    }
    
    // Initialize LED Segments
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
    nightLoad->setExcludeLast(true); // Letzte LED leuchtet statisch als Lampe
    nightLoad->setLastPixelColor(ColorWhite); // Nur die Lampe leuchtet weiß
    nightLoad->setPulse(true);       // Die LED davor pulsiert (in Gelb/ColorCurrentflow)
    heavyLoad = new LedSegment(&Neopixels, IndicesHeavyLoad, LengthHeavyLoad, ColorCurrentflow);
    
    SystemState currentState;

    while(1) {
        if (xSemaphoreTake(NeoPixelMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // 1. Daten sicher aus dem Shared Memory holen
            if(xSemaphoreTake(dataMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                currentState = sysState; 
                xSemaphoreGive(dataMutex);
            } else {
                #ifdef DEBUG
                Serial.println("Failed to acquire data mutex in Neopixel Task");
                #endif
            }
           
            // =========================================================
            // TEIL A: SOLAR
            // =========================================================
            float currentSolar = currentState.current_mA[0];
            int activeSolarCount = currentState.solarActiveCount;
            int solarAnimSpeed = 0; 

            // 1. Basis-Geschwindigkeit berechnen
            if (currentSolar >= 1.0) {
                float clampedCurrent = constrain(currentSolar, 1.0, 40.0);
                solarAnimSpeed = map((long)clampedCurrent, 1, 40, 200, 30);
            }

            // 2. Einzelne Module animieren (nur wenn aktiv)
            for(int i=0; i<4; i++) {
                if (i < activeSolarCount) {
                    SolarModules[i]->setFlow(solarAnimSpeed, false);
                } else {
                    SolarModules[i]->setFlow(0, false);
                    SolarModules[i]->clear();
                }
            }

            // 3. Hauptleitung (allSolarModules) beschleunigen je nach Anzahl
            if (activeSolarCount > 0 && solarAnimSpeed > 0) {
                int combinedSpeed = solarAnimSpeed / activeSolarCount;
                if (combinedSpeed < 20) combinedSpeed = 20; 
                AllSolarModulesIndices->setFlow(combinedSpeed, false);
            } else {
                AllSolarModulesIndices->setFlow(0, false);
                AllSolarModulesIndices->clear();
            }


            // =========================================================
            // TEIL B: KONDENSATOREN
            // =========================================================
            
            // --- B1. Hauptleitung (allCapacitors) Animation ---
            float currentBat = abs(currentState.current_mA[1]); 
            int activeBatCount = currentState.batteryActiveCount;
            int batAnimSpeed = 0;

            if (currentBat >= 1.0) {
                float clampedBat = constrain(currentBat, 1.0, 100.0); 
                // NEU: Werte erhöht. 
                // 400ms (sehr langsam bei 1mA) bis 60ms (schnell bei Vollast 100mA)
                batAnimSpeed = map((long)clampedBat, 1, 100, 400, 60);
            }

            if (activeBatCount > 0 && batAnimSpeed > 0) {
                // NEU: Wir teilen nicht mehr durch activeBatCount, damit 
                // das Lauflicht nicht unnatürlich schnell wird, wenn mehrere 
                // Kondensatoren zugeschaltet sind.
                int combinedBatSpeed = batAnimSpeed; 
                
                // Untergrenze (maximaler Speed) zur Sicherheit auf 40ms gesetzt
                if (combinedBatSpeed < 40) combinedBatSpeed = 40; 
                AllCapacitorsIndices->setFlow(combinedBatSpeed, false);
            } else {
                AllCapacitorsIndices->setFlow(0, false);
                AllCapacitorsIndices->clear();
            }

            // --- B2. Füllstandsanzeige der einzelnen Kondensatoren ---
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

            // ... (Vorheriger Code für percentage bleibt gleich)
            if (percentage < 0.0) percentage = 0.0;
            if (percentage > 1.0) percentage = 1.0;

            // --- NEU: Fließender Farb-Übergang (Rot -> Gelb -> Grün) ---
            uint8_t r, g;
            uint8_t b = 0; // Blau brauchen wir für diese Farben nicht

            if (percentage <= 0.5) {
                // Untere Hälfte (0% bis 50%): Rot bleibt voll, Grün blendet sanft ein
                // percentage * 2.0 rechnet den Bereich 0.0-0.5 auf 0.0-1.0 hoch
                r = 255;
                g = (uint8_t)(255.0 * (percentage * 2.0)); 
            } else {
                // Obere Hälfte (50% bis 100%): Grün ist voll, Rot blendet sanft aus
                // (1.0 - percentage) * 2.0 rechnet den Restbereich rückwärts auf 0.0-1.0
                r = (uint8_t)(255.0 * ((1.0 - percentage) * 2.0));
                g = 255;
            }

            uint32_t batColor = Neopixels.Color(r, g, b);
            // -----------------------------------------------------------

            for(int i=0; i<4; i++) {
                Capacitors[i]->setFlow(0, false);

                if (i < activeBatCount) {
                    // Wir übergeben die stufenlos berechnete Farbe
                    Capacitors[i]->fill(percentage, batColor);
                } else {
                    Capacitors[i]->clear();
                }
            }


            // =========================================================
            // TEIL C: LOADS
            // =========================================================
            float currentLoadTotal = currentState.current_mA[2]; 
            int loadBaseSpeed = 0;

            if (currentLoadTotal >= 5.0) { 
                float clampedLoad = constrain(currentLoadTotal, 5.0, 700.0);
                loadBaseSpeed = map((long)clampedLoad, 5, 700, 200, 30);
            }

            // 1. Zählen, wie viele Verbraucher an sind
            int activeLoadCount = 0;
            if (currentState.constantLoadOn) activeLoadCount++;
            if (currentState.nightLoadOn) activeLoadCount++;
            if (currentState.heavyLoadOn) activeLoadCount++;

            // Fallback für die Animationsgeschwindigkeit, falls Loads eingeschaltet sind,
            // aber (noch) nicht genug Strom gemessen wird
            int animSpeed = (loadBaseSpeed > 0) ? loadBaseSpeed : 200;

            // 2. AllLoads & AfterBuckBoost: Aktiv, sobald IRGENDEINE Last aktiv ist
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

            // 3. toOtherLoads: Aktiv, sobald Night Load ODER Heavy Load aktiv ist
            if (currentState.nightLoadOn || currentState.heavyLoadOn) {
                toOtherLoads->setFlow(animSpeed, false);
            } else {
                toOtherLoads->setFlow(0, false);
                toOtherLoads->clear();
            }

            // 4. Constant Load
            if (currentState.constantLoadOn) {
                constantLoad->setFlow(animSpeed, false);
            } else {
                constantLoad->setFlow(0, false);
                constantLoad->clear();
            }

            // 5. Night Load
            if (currentState.nightLoadOn) {
                nightLoad->setFlow(animSpeed, false);
            } else {
                nightLoad->setFlow(0, false);
                nightLoad->clear();
            }

            // 6. Heavy Load
            if (currentState.heavyLoadOn) {
                heavyLoad->setFlow(animSpeed, false);
            } else {
                heavyLoad->setFlow(0, false);
                heavyLoad->clear();
            }


            // =========================================================
            // UPDATE & SHOW
            // =========================================================
            
            for(int i=0; i<4; i++) SolarModules[i]->update();
            AllSolarModulesIndices->update();
            
            // AllCapacitorsIndices läuft als Animation:
            AllCapacitorsIndices->update();
            
            // Die neuen Segmente updaten:
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