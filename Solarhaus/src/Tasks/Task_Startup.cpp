#include "tasks.hpp"
#include "shared_data.hpp"
#include "Wire.h"
#include "PinDefinitions.hpp"
#include "DebugConfig.hpp"
#include "Arduino.h"
#include <Adafruit_MCP23X17.h>
#include <Adafruit_INA3221.h>
#include <Adafruit_NeoPixel.h>
#include "Config.hpp"





void Task_Startup(void* pvParameters) {
    #ifdef DEBUG
    Serial.begin(115200);
    delay(3000);
    Serial.println("Startup Task is running...");
    #endif
     

    // Initialize I2C and peripherals
    if(xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // Initialize I2C
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 1700000);
        
        if (!GPIOExpander.begin_I2C()) {
            //while (1); // TODO Error handling
        }
        #ifdef DEBUG
        printGPIOExpanderStatus();
        #endif
        if (!CurrentSensor.begin()) {
            //while (1); // TODO Error handling
        }
        #ifdef DEBUG
        for (int i = 0; i <= 2; i++) {
        float busVoltage = CurrentSensor.getBusVoltage(i);   
        float current_mA = CurrentSensor.getCurrentAmps(i) * 1000; // Convert to mA
            
        Serial.print("Channel "); Serial.print(i);
        Serial.print(": "); Serial.print(busVoltage); Serial.print(" V, ");
        Serial.print(current_mA); Serial.print(" mA");
        float Power_mW = busVoltage * (current_mA / 1000.0) * 1000; // Power in mW
        Serial.print(", Power: "); Serial.print(Power_mW); Serial.println(" mW");
        }
        #endif
        xSemaphoreGive(i2cMutex);
    } else {
        #ifdef DEBUG
        Serial.println("Failed to acquire I2C mutex in Startup Task");
        #endif
        while (1); // TODO Error handling for mutex timeout
    }
    // Set all GPIOs to LOW to ensure a known state at startup
  
    // Neopixel Pin
    pinMode(NEOPIXEL_PIN, OUTPUT);
    digitalWrite(NEOPIXEL_PIN, LOW);

    // Wakeup Pin 
    pinMode(WAKEUP_PIN, INPUT_PULLUP); 

    // Enable Pins
    pinMode(ENABLE_3V3_PIN, OUTPUT);
    digitalWrite(ENABLE_3V3_PIN, HIGH);

    pinMode(ENABLE_BATTERY_PIN, OUTPUT);
    digitalWrite(ENABLE_BATTERY_PIN, HIGH);

    // MPPT PWM Pin
    pinMode(MPPT_PWM_PIN, OUTPUT);
    digitalWrite(MPPT_PWM_PIN, LOW);

    int GPIOExpanderPins[] = {
      SOLAR_CELL_1, SOLAR_CELL_2, SOLAR_CELL_3, SOLAR_CELL_4,
      CAPACITOR_1, CAPACITOR_2, CAPACITOR_3, CAPACITOR_4,
      CONSTANT_LOAD, NIGHT_LOAD, HEAVY_LOAD
    };

    for (int i = 0; i < 11; i++) {
      GPIOExpander.pinMode(GPIOExpanderPins[i], OUTPUT);
      GPIOExpander.digitalWrite(GPIOExpanderPins[i], LOW);
    }

    // Initialize NeoNeopixels
    Neopixels.begin(); 
    Neopixels.clear(); 
    Neopixels.show();
    #ifdef DEBUG
    Serial.println("NeoNeopixels initialized");
    for(int i=0; i<5; i++) {
        for(int i=0; i<NUM_NEOPIXELS; i++) {
            Neopixels.setPixelColor(i, Neopixels.Color(0, 150, 0));
        }
        Neopixels.show();
        vTaskDelay(pdMS_TO_TICKS(100));
        Neopixels.clear();
        Neopixels.show();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    #endif

    xTaskCreate(Task_Power_Sensing, "Power Sensing Task", STACK_SIZE_POWER_SENSING_TASK, NULL, PRIORITY_POWER_SENSING_TASK, &TaskHandle_Power_Sensing);
    xTaskCreate(Task_Control_GPIO, "GPIO Control Task", STACK_SIZE_CONTROL_GPIO_TASK, NULL, PRIORITY_CONTROL_GPIO_TASK, &TaskHandle_Control_GPIO);
    xTaskCreate(Task_Neopixel, "Neopixel Task", STACK_SIZE_NEOPIXEL_TASK, NULL, PRIORITY_NEOPIXEL_TASK, &TaskHandle_Neopixel);
    xTaskCreate(Task_Webserver, "Webserver Task", STACK_SIZE_WEBSERVER_TASK, NULL, PRIORITY_WEBSERVER_TASK, &TaskHandle_Webserver);
    xTaskCreate(Task_MPPT, "MPPT Task", STACK_SIZE_MPPT_TASK, NULL, PRIORITY_MPPT_TASK, &TaskHandle_MPPT);
    xTaskCreate(Task_DeepSleep, "Deep Sleep Task", STACK_SIZE_DEEPSLEEP_TASK, NULL, PRIORITY_DEEPSLEEP_TASK, &TaskHandle_DeepSleep);

    #ifdef DEBUG
    xTaskCreate(Task_Debug, "Debug Task", STACK_SIZE_DEBUG_TASK, NULL, PRIORITY_DEBUG_TASK, &TaskHandle_Debug);
    Serial.println("Startup Task done");
    #endif

    vTaskDelete(NULL);
}



void printGPIOExpanderStatus() {
  int pins[] = {SOLAR_CELL_1, SOLAR_CELL_2, SOLAR_CELL_3, SOLAR_CELL_4, 
                CAPACITOR_1, CAPACITOR_2, CAPACITOR_3, CAPACITOR_4, 
                CONSTANT_LOAD, NIGHT_LOAD, HEAVY_LOAD};
  
  const char* namen[] = {"Solar 1", "Solar 2", "Solar 3", "Solar 4", 
                         "Cap 1", "Cap 2", "Cap 3", "Cap 4", 
                         "Const Load", "Night Load", "Heavy Load"};

  Serial.println("--- MCP23017 Status Report ---");
  for (int i = 0; i < 11; i++) {
    bool state = GPIOExpander.digitalRead(pins[i]);
    Serial.print(namen[i]);
    Serial.print(": ");
    Serial.println(state ? "ON (HIGH)" : "OFF (LOW)");
  }
  Serial.println("------------------------------");
}