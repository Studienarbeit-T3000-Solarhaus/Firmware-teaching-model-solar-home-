#include <Arduino.h>
#include "PinDefinitions.hpp"
#include "DebugConfig.hpp"
#include <Adafruit_INA3221.h>
#include "tasks.hpp"
#include "shared_data.hpp"
#include <Adafruit_MCP23X17.h>
#include <Adafruit_NeoPixel.h>
#include "Config.hpp"

Adafruit_INA3221 CurrentSensor;
Adafruit_MCP23X17 GPIOExpander; 
Adafruit_NeoPixel pixels(NUM_NEOPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
    // init mutexes
    i2cMutex = xSemaphoreCreateMutex();
    // Create Startup task
    xTaskCreate(Task_Startup, "Startup Task", STACK_SIZE_STARTUP_TASK, NULL, PRIORITY_STARTUP_TASK, &TaskHandle_Startup);


}

void loop() {
    // FreeRTOS übernimmt, loop bleibt leer
    vTaskDelay(pdMS_TO_TICKS(1000));
}