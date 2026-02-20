#include "tasks.hpp"
#include "shared_data.hpp"
#include "PinDefinitions.hpp"
#include "Config.hpp"
#include "DebugConfig.hpp"
#include <Arduino.h>

// --- KONFIGURATION FÜR 100k/10k/10k SETUP ---

// Hardware-Setup:
// R_Top = 100k, R_Bottom = 10k -> V_native = 6.6V
// R_Inject = 10k an ESP32
//
// Formel: V_out = 12.6V - 10 * V_inject
//
// PWM 0   (0.0V) -> 12.6V Output -> ZERSTÖRUNG!
// PWM 52  (0.67V)-> 5.9V Output  -> MAXIMALE POWER (Spannungsgrenze Kondensator)
// PWM 100 (1.3V) -> 0.0V Output  -> BUCK AUS (Startzustand)
// PWM 115 (>1.5V)-> Latch-Off    -> CHIP ABSTURZ

const int MPPT_PWM_CHANNEL = 0;
const int MPPT_PWM_FREQ = 20000;
const int MPPT_PWM_RES = 8;

// SICHERHEITSGRENZEN (Nicht ändern ohne Nachrechnen!)
const int PWM_LIMIT_MIN_VOLTAGE = 100; // Entspricht ca. 0V Ausgang (Buck aus)
const int PWM_LIMIT_MAX_VOLTAGE = 52;  // Entspricht ca. 6.0V Ausgang (Max für Kondensator)

// Achtung: Invertierte Logik für den Programmierer!
// Kleinerer PWM Wert = HÖHERE Ausgangsspannung = MEHR Last
// Größerer PWM Wert = NIEDRIGERE Ausgangsspannung = WENIGER Last

void Task_MPPT(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // 1. PWM Setup
    ledcSetup(MPPT_PWM_CHANNEL, MPPT_PWM_FREQ, MPPT_PWM_RES);
    ledcAttachPin(MPPT_PWM_PIN, MPPT_PWM_CHANNEL);

    // 2. Sicherer Startzustand (Buck "Aus")
    // Wir fangen mit hoher Injektion an -> Ausgangsspannung niedrig
    int currentDutyCycle = PWM_LIMIT_MIN_VOLTAGE; // ca. 100
    ledcWrite(MPPT_PWM_CHANNEL, currentDutyCycle);

    // --- Variablen für Spannungs-Sweep Algorithmus ---
    enum SweepState { SWEEP, TRACK };
    SweepState mpptState = SWEEP;
    int sweepPwm = PWM_LIMIT_MAX_VOLTAGE; // Start bei maximaler Last (niedrigste Solarspannung)
    float maxPowerDuringSweep = 0.0;
    int bestPwm = PWM_LIMIT_MIN_VOLTAGE;  // Sicherer Startwert
    int stepsSinceMax = 0;                // Zähler, um "etwas weiter zu suchen"

    while (1) {
        float currentSolarPower = 0.0;
        float currentSolarVoltage = 0.0;
        float outputCapVoltage = 0.0;
        
        bool autoMode = true;
        int manualTarget = 100;
        bool dataValid = false;

        // --- DATEN HOLEN ---
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            // INA3221 Channel 0 = Solar Input
            currentSolarPower = sysState.power_mW[0];
            currentSolarVoltage = sysState.busVoltage[0];
            
            // INA3221 Channel 1 (oder 2?) = Ausgang/Kondensator
            // Bitte prüfen, wo dein Ausgang angeschlossen ist! 
            // Ich nehme hier Index 1 an (Channel 2 des INA).
            outputCapVoltage = sysState.busVoltage[1]; 
            
            // Webserver Steuerung
            autoMode = sysState.mppt_auto_mode;
            manualTarget = sysState.manual_pwm_value;
            
            dataValid = true;
            xSemaphoreGive(dataMutex);
        }

        if (dataValid) {
            
            // --- NOT-AUS: ÜBERSPANNUNGSSCHUTZ ---
            // Wenn Kondensator über 6.1V geht, sofort abregeln!
            if (outputCapVoltage > 6.1) {
                // PWM erhöhen -> Spannung senken
                currentDutyCycle = PWM_LIMIT_MIN_VOLTAGE; // Buck aus (0V Ziel)
                ledcWrite(MPPT_PWM_CHANNEL, currentDutyCycle);
                
                // Algorithmus für diesen Durchlauf abbrechen
                vTaskDelayUntil(&xLastWakeTime, PERIOD_MPPT_TASK);
                continue; 
            }

            // --- STEUERUNG ---
            if (!autoMode) {
                // *** MANUELLER MODUS (Slider) ***
                
                // Der Slider kommt vom Webserver (0-255).
                // Wir müssen ihn sicher begrenzen.
                if (manualTarget < PWM_LIMIT_MAX_VOLTAGE) manualTarget = PWM_LIMIT_MAX_VOLTAGE; // Nicht unter 52
                if (manualTarget > PWM_LIMIT_MIN_VOLTAGE) manualTarget = PWM_LIMIT_MIN_VOLTAGE; // Nicht über 105
                
                currentDutyCycle = manualTarget;
                
            } else {
                // *** AUTO MPPT (Spannungs-Sweep mit Peak-Tracking) ***
                
                // 1. Wenn Solarspannung kritisch tief (< 3.0V), Last wegnehmen
                if (currentSolarVoltage < 3.0) {
                    currentDutyCycle = PWM_LIMIT_MIN_VOLTAGE; // Buck fast aus
                    mpptState = SWEEP;                        // Neustart des Sweeps beim nächsten Mal
                    sweepPwm = PWM_LIMIT_MAX_VOLTAGE;         // Beginne wieder bei hoher Last
                    maxPowerDuringSweep = 0.0;
                    stepsSinceMax = 0;
                } 
                else {
                    if (mpptState == SWEEP) {
                        currentDutyCycle = sweepPwm;

                        // 2. Leistung auswerten und sich den besten Wert "merken"
                        if (currentSolarPower > maxPowerDuringSweep) {
                            maxPowerDuringSweep = currentSolarPower;
                            bestPwm = sweepPwm;
                            stepsSinceMax = 0; // Zähler zurücksetzen, da neues Maximum gefunden
                        } else if (maxPowerDuringSweep > 5.0) {
                            // Leistung sinkt wieder (und wir haben schon ein echtes Maximum > 5mW gesehen)
                            stepsSinceMax++;
                        }

                        // 3. Spannung der Solarzelle kontinuierlich von "null weg" erhöhen 
                        // (PWM erhöhen = weniger Last = höhere Solarspannung)
                        sweepPwm += 1; 

                        // 4. Abbruchkriterium: 
                        // Entweder wir stoßen ans Limit, oder wir haben "noch etwas weiter gesucht" 
                        // (z.B. 5 PWM-Schritte) und die Leistung sinkt nur noch ab.
                        if (sweepPwm > PWM_LIMIT_MIN_VOLTAGE || stepsSinceMax >= 5) {
                            mpptState = TRACK;
                            currentDutyCycle = bestPwm; // Zum gemerkten besten Leistungs-Wert springen
                        }
                    } 
                    else if (mpptState == TRACK) {
                        // 5. Auf dem besten Punkt arbeiten
                        currentDutyCycle = bestPwm;

                        // Wenn die Leistung am optimalen Punkt plötzlich stark einbricht 
                        // (z. B. durch eine neue Wolke), starten wir die Suche komplett neu.
                        if (currentSolarPower < maxPowerDuringSweep * 0.8) {
                            mpptState = SWEEP;
                            sweepPwm = PWM_LIMIT_MAX_VOLTAGE;
                            maxPowerDuringSweep = 0.0;
                            stepsSinceMax = 0;
                        }
                    }
                }
            }

            // --- HARTE LIMITS (CLAMPING) ---
            // Das ist der wichtigste Teil zum Schutz der Hardware!
            if (currentDutyCycle < PWM_LIMIT_MAX_VOLTAGE) currentDutyCycle = PWM_LIMIT_MAX_VOLTAGE; // min 52
            if (currentDutyCycle > PWM_LIMIT_MIN_VOLTAGE) currentDutyCycle = PWM_LIMIT_MIN_VOLTAGE; // max 105

            // --- AUSGABE ---
            ledcWrite(MPPT_PWM_CHANNEL, currentDutyCycle);
            
            // Optional: PWM Wert in sysState schreiben für Debugging/Webseite
            // if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            //    sysState.debug_pwm = currentDutyCycle; // Müsste in shared_data.hpp ergänzt werden
            //    xSemaphoreGive(dataMutex);
            // }
        }
        #ifdef DEBUG
        
        // Debug-Ausgabe
        Serial.print("MPPT | Solar: ");
        Serial.print(currentSolarVoltage, 2);
        Serial.print("V, ");
        Serial.print(currentSolarPower, 1);
        Serial.print("mW | Cap: ");
        Serial.print(outputCapVoltage, 2);
        Serial.print("V | PWM: ");
        Serial.println(currentDutyCycle);
        #endif

        // Zykluszeit einhalten
        vTaskDelayUntil(&xLastWakeTime, PERIOD_MPPT_TASK);
    }
}