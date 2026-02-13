#include "tasks.hpp"
#include "shared_data.hpp"
#include <Arduino.h>

void Task_Power_Sensing(void* pvParameters) {
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    // Set Shunt Resistance to 100mOhm for all channels 
    CurrentSensor.setShuntResistance(0, 0.1);
    CurrentSensor.setShuntResistance(1, 0.1);
    CurrentSensor.setShuntResistance(2, 0.1);
    while (1) {
        float v_temp[3] = {0.0f};
        float i_temp[3] = {0.0f};

        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            
            for (int i = 0; i < 3; i++) {
                v_temp[i] = CurrentSensor.getBusVoltage(i);
                i_temp[i] = CurrentSensor.getCurrentAmps(i) * 1000.0f; 
                
            }

            xSemaphoreGive(i2cMutex); 
        }
        // Store Data in Global State
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            
            for (int i = 0; i < 3; i++) {
                sysState.busVoltage[i] = v_temp[i];
                sysState.current_mA[i] = i_temp[i];
                sysState.power_mW[i] = v_temp[i] * i_temp[i];
            }

            xSemaphoreGive(dataMutex); 
        }

        vTaskDelayUntil(&xLastWakeTime, PERIOD_POWER_SENSING_TASK);
    }
}