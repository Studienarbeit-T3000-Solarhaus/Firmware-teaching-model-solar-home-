#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <Adafruit_INA3221.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_NeoPixel.h>

// Mutexes
extern SemaphoreHandle_t i2cMutex;

// Task Priorities
#define PRIORITY_STARTUP_TASK 5
#define PRIORITY_WEBSERVER_TASK 4

// Task stack sizes
#define STACK_SIZE_STARTUP_TASK 2048
#define STACK_SIZE_WEBSERVER_TASK 4096

// Task frequencies (in Hz)

// Task periods (in FreeRTOS ticks)


// Global shared data 
extern Adafruit_INA3221 CurrentSensor;
extern Adafruit_MCP23X17 GPIOExpander;
extern Adafruit_NeoPixel pixels;