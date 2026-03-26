#include "tasks.hpp"
#include "shared_data.hpp"
#include "PinDefinitions.hpp"
#include <Arduino.h>
#include "DebugConfig.hpp"

/**
 * Task zur direkten Messung der Batteriespannung über den ADC des ESP32-C3.
 * Dies dient als Redundanz oder Alternative zur Messung über den INA3221.
 */
float getLiPoPercentage(float voltage);

void Task_BatteryVoltageMeasurement(void* pvParameters) {
    // ADC-Konfiguration (0-3.3V Bereich beim ESP32-C3)
    analogReadResolution(12); // 12-Bit Auflösung (0-4095)
    
    // Spannungsteiler-Faktoren (Annahme: Widerstandsteiler am BATTERY_VOLTAGE_PIN)
    // Wenn z.B. 100k / 10k genutzt wird, ist der Faktor 11.0
    const float voltageDividerRatio = 2.0f; 
    const float referenceVoltage = 3.3f;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 10 Hz Messrate

    while (1) {
        // Mehrfaches Lesen für stabilere Werte (Oversampling)
        long sum = 0;
        const int samples = 16;
        for (int i = 0; i < samples; i++) {
            sum += analogRead(BATTERY_VOLTAGE_PIN);
        }
        float avgAdc = (float)sum / samples;

        // Berechnung der realen Spannung
        float measuredVoltage = ((avgAdc / 4095.0f) * referenceVoltage * voltageDividerRatio) - 0.29f; // Offset 
        
        // --- NEU: In Prozent umrechnen ---
        float batteryPercentage = getLiPoPercentage(measuredVoltage);

        // --- Wert in den globalen Speicher schreiben ---
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            sysState.adcBatteryVoltage = measuredVoltage;
            sysState.adcBatteryPercentage = batteryPercentage; // <--- NEU
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


float getLiPoPercentage(float voltage) {
    // Stützstellen der LiPo Entladekurve (für 1S, anpassen falls 2S/3S)
    const int numPoints = 11;
    const float voltages[numPoints] = {3.20, 3.50, 3.60, 3.70, 3.75, 3.80, 3.85, 3.90, 4.00, 4.10, 4.20};
    const float percentages[numPoints] = {0.0, 5.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 80.0, 90.0, 100.0};

    // Wertebereich abfangen
    if (voltage <= voltages[0]) return 0.0f;
    if (voltage >= voltages[numPoints - 1]) return 100.0f;

    // Lineare Interpolation zwischen den passenden Stützstellen
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