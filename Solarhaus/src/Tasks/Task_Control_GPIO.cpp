#include "tasks.hpp"
#include "shared_data.hpp"
#include "PinDefinitions.hpp"
#include "Config.hpp"
#include "DebugConfig.hpp"
#include <Arduino.h>

// Central control task: reads desired system state, runs simulation logic, and drives GPIO outputs
void Task_Control_GPIO(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    static unsigned long lastLogTime = 0;
    SystemState desiredState;

    while (1) {
        bool gotData = false;

        // Snapshot shared state (short mutex hold time)
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            desiredState = sysState;
            gotData = true;
            xSemaphoreGive(dataMutex);
        }

        // === Day/Night simulation scheduler ===
        if (gotData && desiredState.isSimActive) {
            unsigned long currentMillis = millis();
            unsigned long durationMillis = (desiredState.isDayPhase ? desiredState.dayDurationSec : desiredState.nightDurationSec) * 1000UL;

            // Map real elapsed time to simulated 24h clock
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

            // Helper: checks if current simulated time falls within a scheduled window
            auto isTimeActive = [](int currentMins, int startH, int startM, int endH, int endM) {
                int startMins = startH * 60 + startM;
                int endMins = endH * 60 + endM;
                if (startMins <= endMins) return (currentMins >= startMins && currentMins < endMins); 
                else return (currentMins >= startMins || currentMins < endMins); 
            };

            // Evaluate scheduled loads based on simulated time
            desiredState.constantLoadOn = desiredState.schedConstActive ? isTimeActive(currentMinsTotal, desiredState.schedConstStartH, desiredState.schedConstStartM, desiredState.schedConstEndH, desiredState.schedConstEndM) : false;
            desiredState.nightLoadOn    = desiredState.schedNightActive ? isTimeActive(currentMinsTotal, desiredState.schedNightStartH, desiredState.schedNightStartM, desiredState.schedNightEndH, desiredState.schedNightEndM) : false;
            desiredState.heavyLoadOn    = desiredState.schedHeavyActive ? isTimeActive(currentMinsTotal, desiredState.schedHeavyStartH, desiredState.schedHeavyStartM, desiredState.schedHeavyEndH, desiredState.schedHeavyEndM) : false;

            // Publish scheduled load states for live web UI feedback
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                sysState.constantLoadOn = desiredState.constantLoadOn;
                sysState.nightLoadOn    = desiredState.nightLoadOn;
                sysState.heavyLoadOn    = desiredState.heavyLoadOn;
                xSemaphoreGive(dataMutex);
            }

            // Log simulation data once per second (max 3600 points = 1 hour buffer)
            if (currentMillis - lastLogTime >= 1000) {
                lastLogTime = currentMillis;
                
                SimDataPoint pt;
                pt.timestamp = currentMillis - desiredState.simTimerStart;
                pt.vSolar = desiredState.busVoltage[0];
                pt.iSolar = desiredState.current_mA[0];
                pt.vBat = desiredState.busVoltage[1];
                pt.iBat = desiredState.current_mA[1];

                if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(10))) {
                    if (simulationLog.size() < 3600) {
                        simulationLog.push_back(pt);
                    }
                    xSemaphoreGive(logMutex);
                }
            }
            
            // Phase transition: handle day->night, night->day, or simulation end
            if (currentMillis - desiredState.simTimerStart >= durationMillis) {
                
                if (desiredState.isDayPhase) {
                    desiredState.isDayPhase = false; 
                    desiredState.simTimerStart = currentMillis;
                    desiredState.solarActiveCount = 0;
                    desiredState.batteryActiveCount = desiredState.configBatteryCount;
                    
                } else {
                    if (desiredState.currentCycle >= desiredState.targetCycles) {
                        desiredState.isSimActive = false;
                        desiredState.solarActiveCount = 0; 
                        desiredState.constantLoadOn = false;
                    } else {
                        desiredState.isDayPhase = true;
                        desiredState.currentCycle++;
                        desiredState.simTimerStart = currentMillis;
                        desiredState.solarActiveCount = desiredState.configSolarCount;
                        desiredState.batteryActiveCount = desiredState.configBatteryCount;
                    }
                }

                // Commit phase transition to shared state
                if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                    sysState.isSimActive = desiredState.isSimActive;
                    sysState.isDayPhase = desiredState.isDayPhase;
                    sysState.simTimerStart = desiredState.simTimerStart;
                    sysState.currentCycle = desiredState.currentCycle;
                    sysState.solarActiveCount = desiredState.solarActiveCount;
                    sysState.batteryActiveCount = desiredState.batteryActiveCount;
                    sysState.constantLoadOn = desiredState.constantLoadOn;
                    xSemaphoreGive(dataMutex);
                }
            }
        }

        // === Hardware output stage ===
        if (gotData) {

            // Undervoltage protection: disable all loads if battery bus drops below 1.1V
            if (desiredState.busVoltage[1] < 1.1) {
                desiredState.constantLoadOn = false;
                desiredState.nightLoadOn = false;
                desiredState.heavyLoadOn = false;

                if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                    sysState.constantLoadOn = false;
                    sysState.nightLoadOn = false;
                    sysState.heavyLoadOn = false;
                    xSemaphoreGive(dataMutex);
                }
            }

            // Overvoltage protection: disconnect solar when capacitor bank is fully charged
            float maxCapVoltage = 0;
            switch(desiredState.batteryActiveCount) {
                case 1: maxCapVoltage = 3.17; break;
                case 2: maxCapVoltage = 4.33; break;
                case 3: maxCapVoltage = 5.23; break;
                case 4: maxCapVoltage = 6.1; break;
                default: maxCapVoltage = 6.1; break;
            }

            if (desiredState.busVoltage[1] >= maxCapVoltage && desiredState.solarActiveCount > 0) {
                desiredState.solarActiveCount = 0;
                
                #ifdef DEBUG
                Serial.println("Capacitance reached for active layout, turning off solar cells");
                #endif
            }

            // Drive MCP23017 GPIO expander over I2C
            if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                if(desiredState.busVoltage[1] > 6.1) {
                    desiredState.solarActiveCount = 0;
                    #ifdef DEBUG
                    Serial.println("Capacitance reached, turning off solar cells");
                    #endif
                }

                // Solar cell relay control (incremental: activate first N cells)
                for (int i = 0; i < 4; i++) {
                    bool state = (i < desiredState.solarActiveCount);
                    GPIOExpander.digitalWrite(SOLAR_CELL_1 + i, state ? HIGH : LOW);
                }

                // MPPT bypass relay
                if (desiredState.mpptBypassOn) {
                    GPIOExpander.digitalWrite(BYPASS_MPPT, HIGH); 
                } else {
                    GPIOExpander.digitalWrite(BYPASS_MPPT, LOW);
                }

                // Load switching
                GPIOExpander.digitalWrite(CONSTANT_LOAD, desiredState.constantLoadOn ? HIGH : LOW);
                GPIOExpander.digitalWrite(HEAVY_LOAD, desiredState.heavyLoadOn ? HIGH : LOW);
                GPIOExpander.digitalWrite(NIGHT_LOAD, desiredState.nightLoadOn ? HIGH : LOW);

                xSemaphoreGive(i2cMutex);
            }
        }

        vTaskDelayUntil(&xLastWakeTime, PERIOD_CONTROL_GPIO_TASK);
    }
}