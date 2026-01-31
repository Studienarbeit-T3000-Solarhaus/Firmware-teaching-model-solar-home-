#include "VoltageReader.h"

QueueHandle_t voltageQueue; 
const int NUM_READINGS = 100;

void ReadVoltageTask(void *parameter) {
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db); 
    for (;;) {
        long sumOfMilliVolts = 0;
        for (int i = 0; i < NUM_READINGS; i++) {
            sumOfMilliVolts += analogReadMilliVolts(VOLTAGE_ADC_PIN);
        }
        float averageVoltage = (sumOfMilliVolts / NUM_READINGS) / 1000.0;
        xQueueOverwrite(voltageQueue, &averageVoltage);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void startVoltageTask() {
    voltageQueue = xQueueCreate(1, sizeof(float));
    xTaskCreate(ReadVoltageTask, "ReadVolt", 4096, NULL, 2, NULL);
}