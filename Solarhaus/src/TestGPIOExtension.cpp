#include "TestGPIOExtension.h"
#include "Pins.h"
#define LED_PIN 0 // Beispiel: Verwende GPA0 des MCP23017
extern Adafruit_MCP23X17 mcp;
extern SemaphoreHandle_t i2cMutex;


void TestGPIOExtensionTask(void *parameter) {
    // Beispiel: Setze GPA0 auf HIGH und LOW im Sekundentakt
    for (;;) {
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            mcp.digitalWrite(SOLAR_PIN_1, HIGH); // Setze GPA0 auf HIGH
            xSemaphoreGive(i2cMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Warte 1 Sekunde
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            mcp.digitalWrite(SOLAR_PIN_1, LOW); // Setze GPA0 auf LOW
            xSemaphoreGive(i2cMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Warte 1 Sekunde
        
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