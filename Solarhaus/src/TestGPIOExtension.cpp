#include "TestGPIOExtension.h"

#define LED_PIN 0 // Beispiel: Verwende GPA0 des MCP23017
extern Adafruit_MCP23X17 mcp;
extern SemaphoreHandle_t i2cMutex;
extern Adafruit_INA219 ina219;

void TestGPIOExtensionTask(void *parameter) {
    // Beispiel: Setze GPA0 auf HIGH und LOW im Sekundentakt
    for (;;) {
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            mcp.digitalWrite(LED_PIN, HIGH); // Setze GPA0 auf HIGH
            xSemaphoreGive(i2cMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Warte 1 Sekunde
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            mcp.digitalWrite(LED_PIN, LOW); // Setze GPA0 auf LOW
            xSemaphoreGive(i2cMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Warte 1 Sekunde
        float current_mA = 0.0;
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            current_mA = ina219.getCurrent_mA();
            xSemaphoreGive(i2cMutex);
        }
        
        Serial.print(" | Strom: ");
        Serial.print(current_mA);
        Serial.println(" mA");
    }
}

void startTestGPIOExtensionTask() {
    xTaskCreate(
        TestGPIOExtensionTask,   // Task-Funktion
        "TestGPIOExt",           // Name der Task (für Debugging)
        2048,                    // Stack-Größe in Bytes
        NULL,                    // Parameter für die Task
        10,                       // Priorität der Task
        NULL                     // Task-Handle
    );
}