#include "tasks.hpp"
#include "shared_data.hpp"
#include "PinDefinitions.hpp"
#include "Config.hpp"
#include "DebugConfig.hpp"
#include <Arduino.h>

void Task_Control_GPIO(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    static unsigned long lastLogTime = 0;
    // Lokale Kopie des Status, um Mutex-Zeit kurz zu halten
    SystemState desiredState;

    while (1) {
        bool gotData = false;

        // 1. Gewünschten Systemzustand aus dem Shared Memory lesen
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            desiredState = sysState;
            gotData = true;
            xSemaphoreGive(dataMutex);
        }

        // --- SIMULATION LOGIK ---
        if (gotData && desiredState.isSimActive) {
            unsigned long currentMillis = millis();
            unsigned long durationMillis = (desiredState.isDayPhase ? desiredState.dayDurationSec : desiredState.nightDurationSec) * 1000UL;

            // =========================================================
            // NEU: SCHEDULER (Fiktive Zeit berechnen und Lasten schalten)
            // =========================================================
            float progress = 0.0;
            if (durationMillis > 0) {
                progress = (float)(currentMillis - desiredState.simTimerStart) / (float)durationMillis;
                if (progress > 1.0) progress = 1.0;
            }

            float simTimeFloat = 0;
            if (desiredState.isDayPhase) simTimeFloat = 6.0 + (progress * 12.0);
            else {
                simTimeFloat = 18.0 + (progress * 12.0);
                if (simTimeFloat >= 24.0) simTimeFloat -= 24.0;
            }
            int currentMinsTotal = (int)simTimeFloat * 60 + (int)((simTimeFloat - (int)simTimeFloat) * 60);

            // Helfer-Funktion zum Prüfen des Zeitfensters
            auto isTimeActive = [](int currentMins, int startH, int startM, int endH, int endM) {
                int startMins = startH * 60 + startM;
                int endMins = endH * 60 + endM;
                if (startMins <= endMins) return (currentMins >= startMins && currentMins < endMins); 
                else return (currentMins >= startMins || currentMins < endMins); 
            };

            // Zustand lokal berechnen
            desiredState.constantLoadOn = desiredState.schedConstActive ? isTimeActive(currentMinsTotal, desiredState.schedConstStartH, desiredState.schedConstStartM, desiredState.schedConstEndH, desiredState.schedConstEndM) : false;
            desiredState.nightLoadOn    = desiredState.schedNightActive ? isTimeActive(currentMinsTotal, desiredState.schedNightStartH, desiredState.schedNightStartM, desiredState.schedNightEndH, desiredState.schedNightEndM) : false;
            desiredState.heavyLoadOn    = desiredState.schedHeavyActive ? isTimeActive(currentMinsTotal, desiredState.schedHeavyStartH, desiredState.schedHeavyStartM, desiredState.schedHeavyEndH, desiredState.schedHeavyEndM) : false;

            // Zustand sofort in den globalen Speicher übernehmen, damit das Webinterface es live sieht
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                sysState.constantLoadOn = desiredState.constantLoadOn;
                sysState.nightLoadOn    = desiredState.nightLoadOn;
                sysState.heavyLoadOn    = desiredState.heavyLoadOn;
                xSemaphoreGive(dataMutex);
            }
            // =========================================================
            // --- NEU: LOGGING (Jede Sekunde) ---
            if (currentMillis - lastLogTime >= 1000) {
                lastLogTime = currentMillis;
                
                SimDataPoint pt;
                pt.timestamp = currentMillis - desiredState.simTimerStart; // Relative Zeit wäre schöner, aber absolut ist auch ok
                // Wir nutzen hier einfach millis() oder relative Zeit innerhalb der Phase?
                // Einfacher für den Plot: Laufzeit seit Beginn der Simulation session?
                // Da wir simTimerStart bei Phasenwechsel resetten, ist das schwierig.
                // Nehmen wir einfach eine fortlaufende Nummer oder Zeit.
                // Besser: Wir loggen einfach, Frontend kümmert sich um X-Achse.
                
                pt.vSolar = desiredState.busVoltage[0];
                pt.iSolar = desiredState.current_mA[0];
                pt.vBat = desiredState.busVoltage[1];
                pt.iBat = desiredState.current_mA[1];

                if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(10))) {
                    // Speicher begrenzen (z.B. max 3600 Punkte = 1 Stunde)
                    if (simulationLog.size() < 3600) {
                        simulationLog.push_back(pt);
                    }
                    xSemaphoreGive(logMutex);
                }
            }
            
            // Zeit abgelaufen?
            if (currentMillis - desiredState.simTimerStart >= durationMillis) {
                
                // Entscheidung: Was passiert als nächstes?
                if (desiredState.isDayPhase) {
                    // Tag ist vorbei -> Es wird Nacht
                    desiredState.isDayPhase = false; 
                    desiredState.simTimerStart = currentMillis;
                    
                    // Nacht-Zustand: Solar aus, Licht an (optional, hier lassen wir User-Licht an, aber Solar MUSS aus)
                    desiredState.solarActiveCount = 0;
                    desiredState.batteryActiveCount = desiredState.configBatteryCount;
                    
                    
                } else {
                    // Nacht ist vorbei -> Zyklus zu Ende oder neuer Tag?
                    if (desiredState.currentCycle >= desiredState.targetCycles) {
                        // ZIEL ERREICHT -> STOP
                        desiredState.isSimActive = false;
                        // Optional: Alles ausschalten oder so lassen? 
                        // Wir lassen es meist so oder setzen Solar auf 0.
                        desiredState.solarActiveCount = 0; 
                        desiredState.constantLoadOn = false; // <--- NEU: Nach Simulation ausschalten
                    } else {
                        // Weiter geht's: Neuer Tag, neuer Zyklus
                        desiredState.isDayPhase = true;
                        desiredState.currentCycle++;
                        desiredState.simTimerStart = currentMillis;
                        
                        // Tag-Zustand: Solar wieder an
                        desiredState.solarActiveCount = desiredState.configSolarCount;
                        desiredState.batteryActiveCount = desiredState.configBatteryCount;
                        // <--- NEU: Am Tag die Constant Load wieder ausschalten
                        
                    }
                }

                // Änderungen zurückschreiben
                if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                    sysState.isSimActive = desiredState.isSimActive; // Wichtig falls gestoppt wurde
                    sysState.isDayPhase = desiredState.isDayPhase;
                    sysState.simTimerStart = desiredState.simTimerStart;
                    sysState.currentCycle = desiredState.currentCycle;
                    sysState.solarActiveCount = desiredState.solarActiveCount;
                    sysState.batteryActiveCount = desiredState.batteryActiveCount;
                    // <--- NEU: Geänderten Zustand der Last in den Globalen Status übernehmen
                    sysState.constantLoadOn = desiredState.constantLoadOn;
                    xSemaphoreGive(dataMutex);
                }
            }
        }

        // 2. Hardware schalten (nur wenn Daten gelesen wurden)
        if (gotData) {

            // --- NEU: UNTERSPANNUNGSSCHUTZ ---
            // Wenn Spannung an Channel 1 unter 1.1V fällt -> Alles aus
            if (desiredState.busVoltage[1] < 1.1) {
                // Lokale Steuerungsvariablen auf false setzen (Hardware schaltet gleich aus)
                desiredState.constantLoadOn = false;
                desiredState.nightLoadOn = false;
                desiredState.heavyLoadOn = false;

                // WICHTIG: Auch den globalen Shared Memory aktualisieren.
                // Sonst würde das Webinterface noch "AN" anzeigen oder beim nächsten
                // Klick die Last wieder aktivieren.
                if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                    sysState.constantLoadOn = false;
                    sysState.nightLoadOn = false;
                    sysState.heavyLoadOn = false;
                    xSemaphoreGive(dataMutex);
                }
                
                #ifdef DEBUG
                // Optional: Ausgabe zur Diagnose, falls gewünscht
                // Serial.println("Undervoltage protection active (< 1.1V)! Loads disabled.");
                #endif
            }
            // ---------------------------------

            // --- NEU: DYNAMISCHER ÜBERSPANNUNGSSCHUTZ FÜR SOLARZELLEN ---
            float maxCapVoltage = 0;
            switch(desiredState.batteryActiveCount) {
                case 1: maxCapVoltage = 3.17; break;
                case 2: maxCapVoltage = 4.33; break;
                case 3: maxCapVoltage = 5.23; break;
                case 4: maxCapVoltage = 6.1; break;
                default: maxCapVoltage = 6.1; break;
            }

            // Wenn Spannung Limit erreicht -> Solarzellen abschalten
            if (desiredState.busVoltage[1] >= maxCapVoltage && desiredState.solarActiveCount > 0) {
                desiredState.solarActiveCount = 0; // Lokalen Status updaten (für die Hardware)

                //// WICHTIG: Globalen Status für die Webseite updaten, damit die UI "0 / 4" anzeigt
                //if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                //    sysState.solarActiveCount = 0;
                //    xSemaphoreGive(dataMutex);
                //}
                
                #ifdef DEBUG
                Serial.println("Capacitance reached for active layout, turning off solar cells");
                #endif
            }
            // -------------------------------------------------------------

            // Da der MCP23017 über I2C läuft, brauchen wir den I2C Mutex
            if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                if(desiredState.busVoltage[1] > 6.1) {
                    desiredState.solarActiveCount = 0;
                    #ifdef DEBUG
                    Serial.println("Capacitance reached, turning off solar cells");
                    #endif
                }
                // --- Solar Cells Steuerung (Inkrementell) ---
                for (int i = 0; i < 4; i++) {
                    // Pins: SOLAR_CELL_1 bis SOLAR_CELL_4
                    // Wenn i < Anzahl der aktiven, dann HIGH, sonst LOW
                    bool state = (i < desiredState.solarActiveCount);
                    GPIOExpander.digitalWrite(SOLAR_CELL_1 + i, state ? HIGH : LOW);
                }

                // Annahme: PIN_MPPT_BYPASS ist in deiner PinDefinitions.hpp definiert
                if (desiredState.mpptBypassOn) {
                    GPIOExpander.digitalWrite(BYPASS_MPPT, HIGH); 
                } else {
                    GPIOExpander.digitalWrite(BYPASS_MPPT, LOW);
                }

                // --- Batterie/Kondensator Steuerung (Inkrementell) ---
                //for (int i = 0; i < 4; i++) {
                //    // Pins: CAPACITOR_1 bis CAPACITOR_4
                //    bool state = (i < desiredState.batteryActiveCount);
                //    GPIOExpander.digitalWrite(CAPACITOR_1 + i, state ? HIGH : LOW);
                //}

                // --- Lasten Schalten ---
                GPIOExpander.digitalWrite(CONSTANT_LOAD, desiredState.constantLoadOn ? HIGH : LOW);
                GPIOExpander.digitalWrite(HEAVY_LOAD, desiredState.heavyLoadOn ? HIGH : LOW);
                GPIOExpander.digitalWrite(NIGHT_LOAD, desiredState.nightLoadOn ? HIGH : LOW);

                xSemaphoreGive(i2cMutex);
            }
        }

        // Task-Frequenz einhalten (z.B. 10Hz oder 5Hz reicht für Relais/MOSFETs)
        vTaskDelayUntil(&xLastWakeTime, PERIOD_CONTROL_GPIO_TASK);
    }
}