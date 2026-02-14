#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <Adafruit_INA3221.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_NeoPixel.h>

// Mutexes
extern SemaphoreHandle_t i2cMutex;
extern SemaphoreHandle_t dataMutex;

// Task Priorities
#define PRIORITY_STARTUP_TASK 5
#define PRIORITY_WEBSERVER_TASK 4
#define PRIORITY_POWER_SENSING_TASK 3
#define PRIORITY_CONTROL_GPIO_TASK 3
#define PRIORITY_NEOPIXEL_TASK 2
#define PRIORITY_DEBUG_TASK 1

// Task stack sizes
#define STACK_SIZE_STARTUP_TASK 2048
#define STACK_SIZE_WEBSERVER_TASK 4096
#define STACK_SIZE_POWER_SENSING_TASK 2048
#define STACK_SIZE_CONTROL_GPIO_TASK 2048
#define STACK_SIZE_NEOPIXEL_TASK 4096
#define STACK_SIZE_DEBUG_TASK 2048

// Task frequencies (in Hz)
#define FREQUENCY_POWER_SENSING_TASK 100
#define FREQUENCY_CONTROL_GPIO_TASK 100
#define FREQUENCY_NEOPIXEL_TASK 100

// Task periods (in FreeRTOS ticks)
#define PERIOD_POWER_SENSING_TASK pdMS_TO_TICKS(1000 / FREQUENCY_POWER_SENSING_TASK)
#define PERIOD_CONTROL_GPIO_TASK pdMS_TO_TICKS(1000 / FREQUENCY_CONTROL_GPIO_TASK)
#define PERIOD_NEOPIXEL_TASK pdMS_TO_TICKS(1000 / FREQUENCY_NEOPIXEL_TASK)


// Data Structures
struct SystemState {
    // INA3221 Data
    float busVoltage[3]; 
    float current_mA[3];
    float power_mW[3];
    
    // MCP23017 Status
    int solarActiveCount;
    int batteryActiveCount;
    bool constantLoadOn;
    bool nightLoadOn;
    bool heavyLoadOn;
};

// Global shared data 
extern Adafruit_INA3221 CurrentSensor;
extern Adafruit_MCP23X17 GPIOExpander;
extern Adafruit_NeoPixel Neopixels;
extern SystemState sysState;

