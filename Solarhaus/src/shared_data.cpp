#include "shared_data.hpp"
#include "tasks.hpp"

// Mutexes
SemaphoreHandle_t i2cMutex = NULL;


// Task Handles
TaskHandle_t TaskHandle_Startup = NULL;
