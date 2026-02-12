#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Task function prototypes
void Task_Startup(void* pvParameters);
void Task_WebServer(void* pvParameters);

// Task Handles
extern TaskHandle_t TaskHandle_Startup;

// Function prototypes 
void printGPIOExpanderStatus();

