#include "tasks.hpp"
#include "shared_data.hpp"
#include "PinDefinitions.hpp"
#include "Config.hpp"
#include "DebugConfig.hpp"
#include <Arduino.h>

// PWM controls a buck converter via voltage injection (R_inject = 10k into 100k/10k divider).
// Lower PWM duty = higher output voltage (inverted relationship).
const int MPPT_PWM_CHANNEL = 0;
const int MPPT_PWM_FREQ = 20000;
const int MPPT_PWM_RES = 8;

// Safety bounds: PWM 100 ≈ 0V out (off), PWM 52 ≈ 6V out (capacitor max)
const int PWM_LIMIT_MIN_VOLTAGE = 100;
const int PWM_LIMIT_MAX_VOLTAGE = 52;

// MPPT task: sweeps solar operating point to find peak power, then tracks it
void Task_MPPT(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    ledcSetup(MPPT_PWM_CHANNEL, MPPT_PWM_FREQ, MPPT_PWM_RES);
    ledcAttachPin(MPPT_PWM_PIN, MPPT_PWM_CHANNEL);

    // Start with buck disabled (safe state)
    int currentDutyCycle = PWM_LIMIT_MIN_VOLTAGE;
    ledcWrite(MPPT_PWM_CHANNEL, currentDutyCycle);

    enum SweepState { SWEEP, TRACK };
    SweepState mpptState = SWEEP;
    int sweepPwm = PWM_LIMIT_MAX_VOLTAGE;
    float maxPowerDuringSweep = 0.0;
    int bestPwm = PWM_LIMIT_MIN_VOLTAGE;
    int stepsSinceMax = 0;

    while (1) {
        float currentSolarPower = 0.0;
        float currentSolarVoltage = 0.0;
        float outputCapVoltage = 0.0;
        bool dataValid = false;

        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            currentSolarPower = sysState.power_mW[0];
            currentSolarVoltage = sysState.busVoltage[0];
            outputCapVoltage = sysState.busVoltage[1]; 
            dataValid = true;
            xSemaphoreGive(dataMutex);
        }

        if (dataValid) {
            
            // Overvoltage emergency: immediately shut down buck if cap exceeds 6.1V
            if (outputCapVoltage > 6.1) {
                currentDutyCycle = PWM_LIMIT_MIN_VOLTAGE;
                ledcWrite(MPPT_PWM_CHANNEL, currentDutyCycle);
                vTaskDelayUntil(&xLastWakeTime, PERIOD_MPPT_TASK);
                continue; 
            }

            // If solar voltage collapses, remove load and restart sweep
            if (currentSolarVoltage < 3.0) {
                currentDutyCycle = PWM_LIMIT_MIN_VOLTAGE;
                mpptState = SWEEP;
                sweepPwm = PWM_LIMIT_MAX_VOLTAGE;
                maxPowerDuringSweep = 0.0;
                stepsSinceMax = 0;
            } 
            else {
                if (mpptState == SWEEP) {
                    // Incrementally sweep from max load toward open circuit, tracking peak power
                    currentDutyCycle = sweepPwm;

                    if (currentSolarPower > maxPowerDuringSweep) {
                        maxPowerDuringSweep = currentSolarPower;
                        bestPwm = sweepPwm;
                        stepsSinceMax = 0;
                    } else if (maxPowerDuringSweep > 5.0) {
                        stepsSinceMax++;
                    }

                    sweepPwm += 1; 

                    // End sweep when limit reached or power clearly past peak
                    if (sweepPwm > PWM_LIMIT_MIN_VOLTAGE || stepsSinceMax >= 5) {
                        mpptState = TRACK;
                        currentDutyCycle = bestPwm;
                    }
                } 
                else if (mpptState == TRACK) {
                    // Hold at optimal operating point
                    currentDutyCycle = bestPwm;

                    // Re-sweep if power drops significantly (e.g. shading change)
                    if (currentSolarPower < maxPowerDuringSweep * 0.8) {
                        mpptState = SWEEP;
                        sweepPwm = PWM_LIMIT_MAX_VOLTAGE;
                        maxPowerDuringSweep = 0.0;
                        stepsSinceMax = 0;
                    }
                }
            }

            // Hard clamp PWM to safe hardware limits
            if (currentDutyCycle < PWM_LIMIT_MAX_VOLTAGE) currentDutyCycle = PWM_LIMIT_MAX_VOLTAGE;
            if (currentDutyCycle > PWM_LIMIT_MIN_VOLTAGE) currentDutyCycle = PWM_LIMIT_MIN_VOLTAGE;

            ledcWrite(MPPT_PWM_CHANNEL, currentDutyCycle);
        }

        #ifdef DEBUG
        Serial.print("MPPT | Solar: ");
        Serial.print(currentSolarVoltage, 2);
        Serial.print("V, ");
        Serial.print(currentSolarPower, 1);
        Serial.print("mW | Cap: ");
        Serial.print(outputCapVoltage, 2);
        Serial.print("V | PWM: ");
        Serial.println(currentDutyCycle);
        #endif

        vTaskDelayUntil(&xLastWakeTime, PERIOD_MPPT_TASK);
    }
}
