#include "tasks.hpp"
#include "shared_data.hpp"
#include "PinDefinitions.hpp"
#include "Config.hpp"
#include "DebugConfig.hpp"
#include <Arduino.h>

// PWM Konfiguration für den RC-Filter
const int MPPT_PWM_CHANNEL = 0; // Kanal 0
const int MPPT_PWM_FREQ = 20000; // 20 kHz
const int MPPT_PWM_RES = 8;      // 8 Bit (0-255)

// Limits für die Injektion
// 255 = 3.3V Injektion -> Buck regelt Spannung weit RUNTER (Weniger Last)
// 0   = 0.0V Injektion -> Buck regelt Spannung HOCH (Mehr Last)
const int PWM_MAX_INJECT = 250; // Maximale Drosselung (Fast aus)
const int PWM_MIN_INJECT = 0;   // Maximale Power

void Task_MPPT(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // PWM Setup
    ledcSetup(MPPT_PWM_CHANNEL, MPPT_PWM_FREQ, MPPT_PWM_RES);
    ledcAttachPin(MPPT_PWM_PIN, MPPT_PWM_CHANNEL);

    // Startzustand: Wir fangen "sanft" an
    int currentDutyCycle = 240; 
    ledcWrite(MPPT_PWM_CHANNEL, currentDutyCycle);
    
    // MPPT Variablen
    float prevPower = 0.0;
    // float prevVoltage = 0.0; // Wird aktuell nicht genutzt, aber okay
    int stepSize = 1;     // Wie aggressiv regeln wir?
    bool direction = true; // true = DutyCycle erhöhen (Last senken), false = verringern
    
    // Warte kurz bis System stabil
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        float currentSolarPower = 0.0;
        float currentSolarVoltage = 0.0;
        bool dataValid = false;

        // 1. Aktuelle Werte aus dem Shared Memory holen (ohne I2C Blockade!)
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            // Channel 0 ist Solar laut Task_Webserver.cpp
            currentSolarPower = sysState.power_mW[0];
            currentSolarVoltage = sysState.busVoltage[0];
            dataValid = true;
            xSemaphoreGive(dataMutex);
        }

        if (dataValid) {
            // --- SICHERHEITS-CHECK ---
            // Anpassung für 8V Solarpanel:
            // Wenn die Spannung unter 3.0V fällt (Panel kollabiert), Last wegnehmen.
            // Der alte Wert (9.0) war zu hoch für ein 8V System.
            if (currentSolarVoltage < 3.0) { 
                currentDutyCycle = 250; // Maximale Injektion -> Buck drosseln
                ledcWrite(MPPT_PWM_CHANNEL, currentDutyCycle);
                prevPower = 0; // Algorithmus resetten
                
                #ifdef DEBUG
                // Optional: Info, dass Schutz aktiv ist
                // Serial.println("MPPT: Voltage too low (<3.0V) -> Reset logic");
                #endif
            } 
            else {
                // --- PERTURB & OBSERVE ALGORITHMUS ---
                
                // Leistungsdifferenz
                float powerDiff = currentSolarPower - prevPower;
                
                // Wir regeln nur, wenn sich die Leistung signifikant geändert hat (Rauschen filtern)
                if (abs(powerDiff) > 4.0) { // > 10mW Änderung
                    
                    if (powerDiff > 0) {
                        // Leistung ist GESTIEGEN -> Wir gehen in die richtige Richtung!
                        // Richtung beibehalten.
                    } else {
                        // Leistung ist GESUNKEN -> Falsche Richtung!
                        // Richtung umkehren.
                        direction = !direction;
                    }

                    // Neuen PWM Wert berechnen
                    if (direction) {
                        currentDutyCycle += stepSize; // Mehr Injektion = Weniger Last
                    } else {
                        currentDutyCycle -= stepSize; // Weniger Injektion = Mehr Last
                    }
                }

                // --- LIMITS ---
                if (currentDutyCycle > PWM_MAX_INJECT) {
                    currentDutyCycle = PWM_MAX_INJECT;
                    direction = false; // Zwinge Umkehr beim nächsten Mal
                }
                if (currentDutyCycle < PWM_MIN_INJECT) {
                    currentDutyCycle = PWM_MIN_INJECT;
                    direction = true; // Zwinge Umkehr
                }

                // Neuen Wert schreiben
                ledcWrite(MPPT_PWM_CHANNEL, currentDutyCycle);

                // Werte für nächsten Zyklus speichern
                prevPower = currentSolarPower;
            }

            // --- DEBUG AUSGABE ---
            #ifdef DEBUG
            Serial.print("MPPT Duty Cycle: ");
            Serial.print(currentDutyCycle);
            Serial.print(" | Volt: ");
            Serial.print(currentSolarVoltage);
            Serial.print(" V | Power: ");
            Serial.print(currentSolarPower);
            Serial.println(" mW");
            #endif
        }

        // Frequenz einhalten
        vTaskDelayUntil(&xLastWakeTime, PERIOD_MPPT_TASK);
    }
}