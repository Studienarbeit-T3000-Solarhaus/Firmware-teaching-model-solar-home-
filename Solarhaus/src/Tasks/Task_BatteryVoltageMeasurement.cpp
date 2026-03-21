#include "tasks.hpp"
#include "shared_data.hpp"
#include "PinDefinitions.hpp"
#include <Arduino.h>
#include "DebugConfig.hpp"

/**
 * Task zur direkten Messung der Batteriespannung über den ADC des ESP32-C3.
 * Dies dient als Redundanz oder Alternative zur Messung über den INA3221.
 */
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
        // Spannung = (ADC_Wert / Max_ADC) * Ref_Spannung * Teilerfaktor
        float measuredVoltage = (avgAdc / 4095.0f) * referenceVoltage * voltageDividerRatio;

        // --- NEU: Wert in den globalen Speicher schreiben ---
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            sysState.adcBatteryVoltage = measuredVoltage;
            xSemaphoreGive(dataMutex);
        }
        // ----------------------------------------------------

        #ifdef DEBUG_BatteryVoltageMeasurement
        Serial.print("Battery Voltage Measurement: ");
        Serial.print(measuredVoltage, 3);
        Serial.println(" V");
        #endif

        vTaskDelayUntil(&xLastWakeTime, PERIOD_BATTERY_VOLTAGE_MEASUREMENT_TASK);
    }
}