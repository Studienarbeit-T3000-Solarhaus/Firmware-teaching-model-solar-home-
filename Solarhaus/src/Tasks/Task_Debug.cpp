#include "tasks.hpp"
#include "shared_data.hpp"
#include <Arduino.h>
#include "Config.hpp"
#include "DebugConfig.hpp"

// Task Handle für diesen Task selbst (wird in shared_data.cpp definiert)
extern TaskHandle_t TaskHandle_Debug;

void Task_Debug(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    // Konstante für 10 Sekunden Delay
    const TickType_t xFrequency = pdMS_TO_TICKS(10000); 

    while(1) {
        // Wir warten 10 Sekunden
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

        // Eigener Stack (NULL = aktueller Task)
        Serial.print("Debug Task (Self):   ");
        Serial.println(uxTaskGetStackHighWaterMark(NULL));
        
        // Optional: Freier Heap Speicher gesamt
        Serial.print("System Free Heap:    ");
        Serial.println(esp_get_free_heap_size());
        
        Serial.println("========================================");

        
        extern void printGPIOExpanderStatus();
        printGPIOExpanderStatus();

        
    }
}