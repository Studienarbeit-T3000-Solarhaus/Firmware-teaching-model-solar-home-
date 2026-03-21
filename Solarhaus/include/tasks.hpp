#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Task function prototypes
void Task_Startup(void* pvParameters);
void Task_Webserver(void* pvParameters);
void Task_Power_Sensing(void* pvParameters);
void Task_Control_GPIO(void* pvParameters);
void Task_Neopixel(void* pvParameters);
void Task_Debug(void* pvParameters); 
void Task_MPPT(void* pvParameters);
void Task_DeepSleep(void* pvParameters);
void Task_BatteryVoltageMeasurement(void* pvParameters);

// Task Handles
extern TaskHandle_t TaskHandle_Startup;
extern TaskHandle_t TaskHandle_Webserver;
extern TaskHandle_t TaskHandle_Power_Sensing;
extern TaskHandle_t TaskHandle_Control_GPIO; 
extern TaskHandle_t TaskHandle_Neopixel;
extern TaskHandle_t TaskHandle_Debug;
extern TaskHandle_t TaskHandle_MPPT;
extern TaskHandle_t TaskHandle_DeepSleep;
extern TaskHandle_t TaskHandle_BatteryVoltageMeasurement;

// Function prototypes 
void printGPIOExpanderStatus();

