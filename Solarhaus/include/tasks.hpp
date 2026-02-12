#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Task function prototypes
void Task_Startup(void* pvParameters);
void Task_Webserver(void* pvParameters);
void Task_Power_Sensing(void* pvParameters);
void Task_Control_GPIO(void* pvParameters);
void Task_Neopixel(void* pvParameters);

// Task Handles
extern TaskHandle_t TaskHandle_Startup;
extern TaskHandle_t TaskHandle_Webserver;
extern TaskHandle_t TaskHandle_Power_Sensing;
extern TaskHandle_t TaskHandle_Control_GPIO; 
extern TaskHandle_t TaskHandle_Neopixel;

// Function prototypes 
void printGPIOExpanderStatus();

