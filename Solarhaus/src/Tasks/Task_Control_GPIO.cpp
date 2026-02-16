#include "tasks.hpp"
#include "shared_data.hpp"
#include "PinDefinitions.hpp"
#include "Config.hpp"
#include "DebugConfig.hpp"
#include <Arduino.h>

void Task_Control_GPIO(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

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

        // 2. Hardware schalten (nur wenn Daten gelesen wurden)
        if (gotData) {

            // --- NEU: UNTERSPANNUNGSSCHUTZ ---
            // Wenn Spannung an Channel 0 unter 1.1V fällt -> Alles aus
            if (desiredState.busVoltage[0] < 1.1) {
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

            // Da der MCP23017 über I2C läuft, brauchen wir den I2C Mutex
            if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                if(desiredState.busVoltage[0] > 6) {
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

                // --- Batterie/Kondensator Steuerung (Inkrementell) ---
                for (int i = 0; i < 4; i++) {
                    // Pins: CAPACITOR_1 bis CAPACITOR_4
                    bool state = (i < desiredState.batteryActiveCount);
                    GPIOExpander.digitalWrite(CAPACITOR_1 + i, state ? HIGH : LOW);
                }

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