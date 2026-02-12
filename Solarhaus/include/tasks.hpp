#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Task function prototypes
void Task_Startup(void* pvParameters);
void Task_Webserver(void* pvParameters);

// Task Handles
extern TaskHandle_t TaskHandle_Startup;
extern TaskHandle_t TaskHandle_Webserver;

// Function prototypes 
void printGPIOExpanderStatus();

