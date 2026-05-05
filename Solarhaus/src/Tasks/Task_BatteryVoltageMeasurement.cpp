#include "tasks.hpp"
#include "shared_data.hpp"
#include "PinDefinitions.hpp"
#include <Arduino.h>
#include "DebugConfig.hpp"

float getLiPoPercentage(float voltage);

// Periodically reads battery voltage via ADC with oversampling and publishes voltage + SoC
void Task_BatteryVoltageMeasurement(void* pvParameters) {
    analogReadResolution(12);
    
    const float voltageDividerRatio = 2.0f; 
    const float referenceVoltage = 3.3f;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100);

    while (1) {
        // 16x oversampling for noise reduction
        long sum = 0;
        const int samples = 16;
        for (int i = 0; i < samples; i++) {
            sum += analogRead(BATTERY_VOLTAGE_PIN);
        }
        float avgAdc = (float)sum / samples;

        // Convert ADC reading to actual voltage (accounts for voltage divider and calibration offset)
        float measuredVoltage = ((avgAdc / 4095.0f) * referenceVoltage * voltageDividerRatio) - 0.29f;
        
        float batteryPercentage = getLiPoPercentage(measuredVoltage);

        // Thread-safe update of shared system state
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            sysState.adcBatteryVoltage = measuredVoltage;
            sysState.adcBatteryPercentage = batteryPercentage;
            xSemaphoreGive(dataMutex);
        }

        #ifdef DEBUG_BatteryVoltageMeasurement
        Serial.print("Battery Voltage: ");
        Serial.print(measuredVoltage, 3);
        Serial.print(" V -> ");
        Serial.print(batteryPercentage, 1);
        Serial.println(" %");
        #endif

        vTaskDelayUntil(&xLastWakeTime, PERIOD_BATTERY_VOLTAGE_MEASUREMENT_TASK);
    }
}

// Maps battery voltage to State-of-Charge using linear interpolation on a 1S LiPo discharge curve
float getLiPoPercentage(float voltage) {
    const int numPoints = 11;
    const float voltages[numPoints] = {3.20, 3.50, 3.60, 3.70, 3.75, 3.80, 3.85, 3.90, 4.00, 4.10, 4.20};
    const float percentages[numPoints] = {0.0, 5.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 80.0, 90.0, 100.0};

    if (voltage <= voltages[0]) return 0.0f;
    if (voltage >= voltages[numPoints - 1]) return 100.0f;

    for (int i = 0; i < numPoints - 1; i++) {
        if (voltage >= voltages[i] && voltage <= voltages[i + 1]) {
            float vDiff = voltages[i + 1] - voltages[i];
            float pDiff = percentages[i + 1] - percentages[i];
            float vFraction = (voltage - voltages[i]) / vDiff;
            return percentages[i] + (vFraction * pDiff);
        }
    }
    return 0.0f;
}