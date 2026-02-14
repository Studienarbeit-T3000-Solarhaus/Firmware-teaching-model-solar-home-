#include "tasks.hpp"
#include "shared_data.hpp"
#include "Config.hpp"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LedSegment.hpp"

extern Adafruit_NeoPixel Neopixels;

void Task_Neopixel(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    Neopixels.begin();
    Neopixels.setBrightness(10);
    Neopixels.clear();
    Neopixels.show();

    // Initialize LED Segments
    for(int i=0; i<4; i++) {
        solarModules[i] = new LedSegment(&Neopixels, solarModulesStart[i], solarModulesLengths[i], ColorCurrentflow);
        capacitors[i] = new LedSegment(&Neopixels, capacitorsStart[i], capacitorsLengths[i], ColorChargeCaps);
    }
    allSolarModules = new LedSegment(&Neopixels, allSolarModulesStart, allSolarModulesLength, ColorCurrentflow);
    allCapacitors = new LedSegment(&Neopixels, allCapacitorsStart, allCapacitorsLength, ColorChargeCaps);
    allLoads = new LedSegment(&Neopixels, allLoadsStart, allLoadsLength, ColorDischargeCaps);
    constantLoad = new LedSegment(&Neopixels, loadStart[0], loadLengths[0], ColorCurrentflow);
    nightLoad = new LedSegment(&Neopixels, loadStart[1], loadLengths[1], ColorCurrentflow);
    heavyLoad = new LedSegment(&Neopixels, loadStart[2], loadLengths[2], ColorCurrentflow);

    SystemState currentState;

    while(1) {
       
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
        // TEIL A: SOLAR (Animation basierend auf Stromstärke Ch0)
        // =========================================================
        float currentSolar = currentState.current_mA[0];
        int solarAnimSpeed = 0; 

        if (currentSolar >= 1.0) {
            float clampedCurrent = constrain(currentSolar, 1.0, 40.0);
            solarAnimSpeed = map((long)clampedCurrent, 1, 40, 200, 30);
        }

        for(int i=0; i<4; i++) solarModules[i]->setFlow(solarAnimSpeed, false);
        allSolarModules->setFlow(solarAnimSpeed, false);


        // =========================================================
        // TEIL B: KONDENSATOREN (Einzel-Füllstandsanzeige)
        // =========================================================
        
        // 1. Berechnung des Füllstands & Farbe (Gilt für alle aktiven Kondensatoren)
        float batVoltage = currentState.busVoltage[0]; // Channel 1
        float minV = 1.0;
        float maxV = 6.0;

        float percentage = (batVoltage - minV) / (maxV - minV);
        if (percentage < 0.0) percentage = 0.0;
        if (percentage > 1.0) percentage = 1.0;

        uint32_t batColor;
        if (percentage > 0.5) {
            batColor = Neopixels.Color(0, 119, 187); // Blau
        } else if (percentage > 0.2) {
            batColor = Neopixels.Color(238, 204, 17); // Gelb
        } else {
            batColor = Neopixels.Color(213, 94, 0);   // Rot/Orange
        }

        // 2. Das "Gesamt-Segment" (allCapacitors) ausschalten, damit es nicht stört
        allCapacitors->setFlow(0, false);
        allCapacitors->clear();

        // 3. Durch die 4 einzelnen Kondensator-Segmente iterieren
        int activeCount = currentState.batteryActiveCount; // z.B. 2 bedeutet: Cap 0 und 1 sind an

        for(int i=0; i<4; i++) {
            // Zuerst Animation auf diesem Segment stoppen
            capacitors[i]->setFlow(0, false);

            if (i < activeCount) {
                // Kondensator ist AKTIV -> Füllstand anzeigen
                
                int len = capacitorsLengths[i];   // z.B. 3 LEDs
                int start = capacitorsStart[i];   // Start-Index auf dem Strip

                // Anzahl leuchtender LEDs berechnen
                int ledsOn = (int)(percentage * len);
                // Mindestens 1 LED an bei >1% Ladung, damit man sieht, dass er "lebt"
                if (percentage > 0.01 && ledsOn == 0) ledsOn = 1;

                // Pixel setzen
                for(int j=0; j<len; j++) {
                    if (j < ledsOn) {
                        Neopixels.setPixelColor(start + j, batColor);
                    } else {
                        Neopixels.setPixelColor(start + j, 0); // Rest des Balkens aus
                    }
                }

            } else {
                // Kondensator ist INAKTIV -> Komplett aus
                capacitors[i]->clear();
            }
        }


        // =========================================================
        // TEIL C: LOADS (Animation basierend auf Stromstärke Ch2)
        // =========================================================
        float currentLoadTotal = currentState.current_mA[2]; // Channel 2 = Load
        int loadBaseSpeed = 0;

        // Basis-Geschwindigkeit aus Gesamtstrom berechnen (0 - 700mA)
        if (currentLoadTotal >= 5.0) { // Kleiner Schwellwert gegen Rauschen
            float clampedLoad = constrain(currentLoadTotal, 5.0, 700.0);
            // 5mA = langsam (200ms), 700mA = sehr schnell (30ms)
            loadBaseSpeed = map((long)clampedLoad, 5, 700, 200, 30);
        }

        // 1. Zuleitung (allLoads): Läuft immer mit dem Takt des Gesamtstroms
        allLoads->setFlow(loadBaseSpeed, false);

        // 2. Heavy Load (Waschmaschine):
        // Läuft nur, wenn sie eingeschaltet ist, und dann im Takt des Stroms.
        if (currentState.heavyLoadOn) {
            // Wenn Heavy Load an ist, dominiert sie meist den Stromverbrauch.
            // Wir nutzen die berechnete Geschwindigkeit.
            // Falls Strom < 5mA (z.B. Fehler oder Leerlauf), geben wir Minimalspeed (200ms) statt 'Aus', damit man sieht dass sie an ist.
            int heavySpeed = (loadBaseSpeed > 0) ? loadBaseSpeed : 200;
            heavyLoad->setFlow(heavySpeed, false); 
        } else {
            heavyLoad->setFlow(0, false); // Aus
        }

        // 3. Andere Lasten (Light / Night):
        // Wenn an, nutzen sie auch den "Strom-Takt" oder einen Standardwert, falls Strom sehr gering.
        if (currentState.constantLoadOn) {
            constantLoad->setFlow((loadBaseSpeed > 0) ? loadBaseSpeed : 150, false);
        } else {
            constantLoad->setFlow(0, false);
        }

        if (currentState.nightLoadOn) {
            nightLoad->setFlow((loadBaseSpeed > 0) ? loadBaseSpeed : 150, false);
        } else {
            nightLoad->setFlow(0, false);
        }


        // =========================================================
        // UPDATE & SHOW
        // =========================================================
        
        // Solar
        for(int i=0; i<4; i++) solarModules[i]->update();
        allSolarModules->update();
        
        // Loads
        allLoads->update();
        constantLoad->update();
        nightLoad->update();
        heavyLoad->update();
        
        Neopixels.show();

        vTaskDelayUntil(&xLastWakeTime, PERIOD_NEOPIXEL_TASK);
    }
}