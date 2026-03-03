#include "shared_data.hpp"
#include "tasks.hpp"

// Mutexes
SemaphoreHandle_t i2cMutex = NULL;
SemaphoreHandle_t dataMutex = NULL;
SemaphoreHandle_t logMutex = NULL; // <--- NEU

// Log Vektor
std::vector<SimDataPoint> simulationLog; // <--- NEU



// Task Handles
TaskHandle_t TaskHandle_Startup = NULL;
TaskHandle_t TaskHandle_Webserver = NULL;
TaskHandle_t TaskHandle_Power_Sensing = NULL;
TaskHandle_t TaskHandle_Control_GPIO = NULL;
TaskHandle_t TaskHandle_Neopixel = NULL;
TaskHandle_t TaskHandle_Debug = NULL;
TaskHandle_t TaskHandle_MPPT = NULL;
TaskHandle_t TaskHandle_DeepSleep = NULL; 