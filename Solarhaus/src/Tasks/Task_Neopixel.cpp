#include "tasks.hpp"
#include "shared_data.hpp"
#include "Config.hpp"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LedSegment.hpp"

extern Adafruit_NeoPixel Neopixels;

void Task_Neopixel(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    if (xSemaphoreTake(NeoPixelMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    Neopixels.begin();
    Neopixels.setBrightness(10);
    Neopixels.clear();
    Neopixels.show();
    xSemaphoreGive(NeoPixelMutex);
    }
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

    // Testsegment 0: Cyan, 100ms Geschwindigkeit, vorwärts
    testSegments[0] = new LedSegment(&Neopixels, testIndices[0], testLengths[0], Neopixels.Color(0, 255, 255));
    testSegments[0]->setFlow(100, false);

    // Testsegment 1: Magenta, 100ms Geschwindigkeit, rückwärts (für einen coolen gegenläufigen Effekt)
    testSegments[1] = new LedSegment(&Neopixels, testIndices[1], testLengths[1], Neopixels.Color(255, 0, 255));
    testSegments[1]->setFlow(100, false);

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
                solarModules[i]->setFlow(solarAnimSpeed, false);
            } else {
                solarModules[i]->setFlow(0, false);
                solarModules[i]->clear();
            }
        }

        // 3. Hauptleitung (allSolarModules) beschleunigen je nach Anzahl
        if (activeSolarCount > 0 && solarAnimSpeed > 0) {
            // Zeit durch Anzahl teilen = Schneller
            // Beispiel: 200ms Basis / 4 Module = 50ms (sehr schnell)
            int combinedSpeed = solarAnimSpeed / activeSolarCount;
            // Sicherheitshalber nicht schneller als 20ms, damit man es noch sieht
            if (combinedSpeed < 20) combinedSpeed = 20; 
            allSolarModules->setFlow(combinedSpeed, false);
        } else {
            allSolarModules->setFlow(0, false);
            allSolarModules->clear();
        }


        // =========================================================
        // TEIL B: KONDENSATOREN
        // =========================================================
        
        // --- B1. Hauptleitung (allCapacitors) Animation ---
        float currentBat = abs(currentState.current_mA[1]); // Betrag des Stroms (Laden/Entladen)
        int activeBatCount = currentState.batteryActiveCount;
        int batAnimSpeed = 0;

        // Basis-Geschwindigkeit für Batterie-Strom
        if (currentBat >= 1.0) {
            float clampedBat = constrain(currentBat, 1.0, 100.0); // Skalierung anpassen falls nötig
            batAnimSpeed = map((long)clampedBat, 1, 100, 200, 30);
        }

        // Beschleunigung je nach Anzahl aktiver Kondensatoren
        if (activeBatCount > 0 && batAnimSpeed > 0) {
            int combinedBatSpeed = batAnimSpeed / activeBatCount;
            if (combinedBatSpeed < 20) combinedBatSpeed = 20;
            allCapacitors->setFlow(combinedBatSpeed, false);
        } else {
            allCapacitors->setFlow(0, false);
            allCapacitors->clear();
        }

        // --- B2. Füllstandsanzeige der einzelnen Kondensatoren ---
        float batVoltage = currentState.busVoltage[1]; // Channel prüfen! Ggf. [1] nutzen
        float minV = 1.2;
        float maxV = 6.1;

        // NEU: Spannungsniveau je nach Anzahl der aktiven Akkus anpassen.
        // Passe die Werte hier an deine tatsächliche Hardware an (z.B. Schritte in 2.7V oder 3.0V)
        switch(activeBatCount) {
            case 1: maxV = 3.17;  break; // 1 Akku  -> 100% voll bei 3.0V
            case 2: maxV = 4.33;  break; // 2 Akkus -> 100% voll bei 6.0V
            case 3: maxV = 5.23;  break; // 3 Akkus -> 100% voll bei 9.0V
            case 4: maxV = 6.1; break; // 4 Akkus -> 100% voll bei 12.0V
            default: maxV = 0; break; // Sicherheitshalber, falls Count 0 ist
        }

        float percentage = 0.0;
        if (maxV > minV) {
            percentage = (batVoltage - minV) / (maxV - minV);
        }

        if (percentage < 0.0) percentage = 0.0;
        if (percentage > 1.0) percentage = 1.0;

        uint32_t batColor;
        if (percentage > 0.5) batColor = Neopixels.Color(0, 119, 187);      // Blau
        else if (percentage > 0.2) batColor = Neopixels.Color(238, 204, 17); // Gelb
        else batColor = Neopixels.Color(213, 94, 0);                         // Rot/Orange

        for(int i=0; i<4; i++) {
            // Zuerst Flow stoppen, da wir hier statisch füllen
            capacitors[i]->setFlow(0, false);

            if (i < activeBatCount) {
                // Wir übergeben jetzt einfach direkt den Prozentwert!
                capacitors[i]->fill(percentage, batColor);
            } else {
                capacitors[i]->clear();
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

        // 2. Hauptleitung (allLoads) beschleunigen
        if (activeLoadCount > 0 && loadBaseSpeed > 0) {
            int combinedLoadSpeed = loadBaseSpeed / activeLoadCount;
            if (combinedLoadSpeed < 20) combinedLoadSpeed = 20;
            allLoads->setFlow(combinedLoadSpeed, false);
        } else {
            // Optional: Wenn Strom fließt, aber angeblich keine Last an ist (Leckstrom?), trotzdem langsam laufen lassen?
            // Hier strikt: Wenn Zähler 0, dann aus, oder fallback auf baseSpeed wenn activeLoadCount 0 ist aber Strom da.
            // Wir machen es strikt nach Schalterstellung:
             allLoads->setFlow(0, false);
             allLoads->clear();
        }

        // 3. Heavy Load
        if (currentState.heavyLoadOn) {
            int heavySpeed = (loadBaseSpeed > 0) ? loadBaseSpeed : 200;
            heavyLoad->setFlow(heavySpeed, false); 
        } else {
            heavyLoad->setFlow(0, false); 
            heavyLoad->clear();
        }

        // 4. Constant Load
        if (currentState.constantLoadOn) {
            constantLoad->setFlow((loadBaseSpeed > 0) ? loadBaseSpeed : 150, false);
        } else {
            constantLoad->setFlow(0, false);
            constantLoad->clear();
        }

        // 5. Night Load
        if (currentState.nightLoadOn) {
            nightLoad->setFlow((loadBaseSpeed > 0) ? loadBaseSpeed : 150, false);
        } else {
            nightLoad->setFlow(0, false);
            nightLoad->clear();
        }


        // =========================================================
        // UPDATE & SHOW
        // =========================================================
        
        for(int i=0; i<4; i++) solarModules[i]->update();
        allSolarModules->update();
        
        // Kondensatoren müssen nicht update() rufen für Animation, da sie manuell gesetzt wurden,
        // ABER allCapacitors läuft als Animation:
        allCapacitors->update();
        // Die einzelnen capacitors[i] haben wir oben direkt per setPixelColor gesetzt, 
        // daher kein update() nötig, schadet aber auch nicht (speed ist eh 0).
        
        allLoads->update();
        constantLoad->update();
        nightLoad->update();
        heavyLoad->update();

        for(int i=0; i<2; i++) testSegments[i]->update();
        
        Neopixels.show();
        xSemaphoreGive(NeoPixelMutex);
    }

        vTaskDelayUntil(&xLastWakeTime, PERIOD_NEOPIXEL_TASK);
    }
}