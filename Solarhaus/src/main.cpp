#include <Arduino.h>
#include "PinDefinitions.hpp"
#include "DebugConfig.hpp"
#include <Adafruit_INA3221.h>
#include "tasks.hpp"
#include "shared_data.hpp"
#include <Adafruit_MCP23X17.h>
#include <Adafruit_NeoPixel.h>
#include "Config.hpp"

// Global hardware driver instances and shared system state
Adafruit_INA3221 CurrentSensor;
Adafruit_MCP23X17 GPIOExpander; 
Adafruit_NeoPixel Neopixels(NUM_NEOPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
SystemState sysState;

// Entry point: creates mutexes and launches the startup task; all further work runs under FreeRTOS
void setup() {
    i2cMutex = xSemaphoreCreateMutex();
    dataMutex = xSemaphoreCreateMutex();
    logMutex = xSemaphoreCreateMutex();
    NeoPixelMutex = xSemaphoreCreateMutex();

    xTaskCreate(Task_Startup, "Startup Task", STACK_SIZE_STARTUP_TASK, NULL, PRIORITY_STARTUP_TASK, &TaskHandle_Startup);
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}