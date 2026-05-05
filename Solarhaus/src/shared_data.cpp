#include "shared_data.hpp"
#include "tasks.hpp"

// FreeRTOS synchronization primitives for shared resource access
SemaphoreHandle_t i2cMutex = NULL;
SemaphoreHandle_t dataMutex = NULL;
SemaphoreHandle_t logMutex = NULL;
SemaphoreHandle_t NeoPixelMutex = NULL; 

// Simulation telemetry buffer (consumed by /api/history endpoint)
std::vector<SimDataPoint> simulationLog;

// Task handles for stack monitoring and lifecycle management
TaskHandle_t TaskHandle_Startup = NULL;
TaskHandle_t TaskHandle_Webserver = NULL;
TaskHandle_t TaskHandle_Power_Sensing = NULL;
TaskHandle_t TaskHandle_Control_GPIO = NULL;
TaskHandle_t TaskHandle_Neopixel = NULL;
TaskHandle_t TaskHandle_Debug = NULL;
TaskHandle_t TaskHandle_MPPT = NULL;
TaskHandle_t TaskHandle_DeepSleep = NULL; 
TaskHandle_t TaskHandle_BatteryVoltageMeasurement = NULL; 