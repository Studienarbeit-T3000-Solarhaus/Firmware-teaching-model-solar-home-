#include "tasks.hpp"
#include "shared_data.hpp"
#include <Arduino.h>
#include "Config.hpp"
#include "DebugConfig.hpp"

extern TaskHandle_t TaskHandle_Debug;

// Periodic diagnostics task: prints stack high water marks and heap usage every 10s
void Task_Debug(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10000); 

    while(1) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        Serial.println("========================================");
        Serial.println("   STACK HIGH WATER MARK (Free Bytes)   ");
        Serial.println("========================================");

        if (TaskHandle_Webserver != NULL) {
            Serial.print("Webserver Task:      ");
            Serial.println(uxTaskGetStackHighWaterMark(TaskHandle_Webserver));
        }

        if (TaskHandle_Power_Sensing != NULL) {
            Serial.print("Power Sensing Task:  ");
            Serial.println(uxTaskGetStackHighWaterMark(TaskHandle_Power_Sensing));
        }

        if (TaskHandle_Control_GPIO != NULL) {
            Serial.print("GPIO Control Task:   ");
            Serial.println(uxTaskGetStackHighWaterMark(TaskHandle_Control_GPIO));
        }

        if (TaskHandle_Neopixel != NULL) {
            Serial.print("Neopixel Task:       ");
            Serial.println(uxTaskGetStackHighWaterMark(TaskHandle_Neopixel));
        }

        Serial.print("Debug Task (Self):   ");
        Serial.println(uxTaskGetStackHighWaterMark(NULL));
        
        Serial.print("System Free Heap:    ");
        Serial.println(esp_get_free_heap_size());
        
        Serial.println("========================================");

        extern void printGPIOExpanderStatus();
        printGPIOExpanderStatus();
    }
}