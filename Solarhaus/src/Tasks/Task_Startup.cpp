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

// Hilfsfunktion für fatalen Fehler
void handleFatalI2CError() {
    bool state = false;
    while(1) {
        state = !state;
        for(int i = 0; i < NUM_NEOPIXELS; i++) {
            Neopixels.setPixelColor(i, state ? Neopixels.Color(255, 0, 0) : 0);
        }
        Neopixels.show();
        delay(200); // Schnelles Blinken (200ms an, 200ms aus)
    }
}

void Task_Startup(void* pvParameters) {
    #ifdef DEBUG
    Serial.begin(115200);
    delay(3000);
    Serial.println("Startup Task is running...");
    #endif
     
    // 1. Stromversorgung für Peripherie aktivieren
    pinMode(ENABLE_3V3_PIN, OUTPUT);
    digitalWrite(ENABLE_3V3_PIN, HIGH);

    pinMode(ENABLE_BATTERY_PIN, OUTPUT);
    digitalWrite(ENABLE_BATTERY_PIN, HIGH);
    
    // Neopixel Pin initialisieren
    pinMode(NEOPIXEL_PIN, OUTPUT);
    digitalWrite(NEOPIXEL_PIN, LOW);

    // 2. NeoPixel SOFORT initialisieren (für Fehlermeldungen)
    Neopixels.begin();
    Neopixels.setBrightness(255);
    Neopixels.clear();
    Neopixels.show();

    delay(100); // Stabilisierung abwarten

    // 3. I2C und Peripherie initialisieren
    bool i2cFatal = false;

    if(xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 1700000);
        delay(100);
        
        if (!GPIOExpander.begin_I2C()) {
            i2cFatal = true;
            Serial.println("FATAL: MCP23017 not found!");
        }
        
        if (!i2cFatal && !CurrentSensor.begin()) {
            i2cFatal = true;
            Serial.println("FATAL: INA3221 not found!");
        }
        
        xSemaphoreGive(i2cMutex);
    } else {
        i2cFatal = true;
    }

    // Wenn I2C Fehler -> Sofort blinken und hier stoppen!
    if (i2cFatal) {
        handleFatalI2CError();
    }

    #ifdef DEBUG
    printGPIOExpanderStatus();
    #endif

    // Normaler Startup geht weiter...
    pinMode(WAKEUP_PIN, INPUT_PULLUP); 
    pinMode(MPPT_PWM_PIN, OUTPUT);
    digitalWrite(MPPT_PWM_PIN, LOW);

    int GPIOExpanderPins[] = {
      SOLAR_CELL_1, SOLAR_CELL_2, SOLAR_CELL_3, SOLAR_CELL_4, 
      ENABLE_BUCK_BOOST_CONVERTER, BYPASS_MPPT, CAPACITOR_3, CAPACITOR_4,
      CONSTANT_LOAD, NIGHT_LOAD, HEAVY_LOAD
    };

    for (int i = 0; i < 11; i++) {
      GPIOExpander.pinMode(GPIOExpanderPins[i], OUTPUT);
      GPIOExpander.digitalWrite(GPIOExpanderPins[i], LOW);
    }

    GPIOExpander.digitalWrite(ENABLE_BUCK_BOOST_CONVERTER, HIGH); 
    GPIOExpander.digitalWrite(BYPASS_MPPT, LOW);

    

    #ifdef DEBUG
    Serial.println("NeoNeopixels Startup-Check (Green)");
    for(int i=0; i<3; i++) {
        for(int j=0; j<NUM_NEOPIXELS; j++) Neopixels.setPixelColor(j, Neopixels.Color(0, 100, 0));
        Neopixels.show();
        vTaskDelay(pdMS_TO_TICKS(100));
        Neopixels.clear();
        Neopixels.show();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    #endif

    // Tasks erst jetzt erstellen
    xTaskCreate(Task_Power_Sensing, "Power Sensing Task", STACK_SIZE_POWER_SENSING_TASK, NULL, PRIORITY_POWER_SENSING_TASK, &TaskHandle_Power_Sensing);
    xTaskCreate(Task_Control_GPIO, "GPIO Control Task", STACK_SIZE_CONTROL_GPIO_TASK, NULL, PRIORITY_CONTROL_GPIO_TASK, &TaskHandle_Control_GPIO);
    xTaskCreate(Task_Neopixel, "Neopixel Task", STACK_SIZE_NEOPIXEL_TASK, NULL, PRIORITY_NEOPIXEL_TASK, &TaskHandle_Neopixel);
    xTaskCreate(Task_Webserver, "Webserver Task", STACK_SIZE_WEBSERVER_TASK, NULL, PRIORITY_WEBSERVER_TASK, &TaskHandle_Webserver);
    xTaskCreate(Task_MPPT, "MPPT Task", STACK_SIZE_MPPT_TASK, NULL, PRIORITY_MPPT_TASK, &TaskHandle_MPPT);
    xTaskCreate(Task_DeepSleep, "Deep Sleep Task", STACK_SIZE_DEEPSLEEP_TASK, NULL, PRIORITY_DEEPSLEEP_TASK, &TaskHandle_DeepSleep);
    xTaskCreate(Task_BatteryVoltageMeasurement, "Battery Voltage Measurement Task", STACK_SIZE_BATTERY_VOLTAGE_MEASUREMENT_TASK, NULL, PRIORITY_BATTERY_VOLTAGE_MEASUREMENT_TASK, &TaskHandle_BatteryVoltageMeasurement);

    #ifdef DEBUG
    xTaskCreate(Task_Debug, "Debug Task", STACK_SIZE_DEBUG_TASK, NULL, PRIORITY_DEBUG_TASK, &TaskHandle_Debug);
    Serial.println("Startup Task done");
    #endif

    vTaskDelete(NULL);
}

void printGPIOExpanderStatus() {
  int pins[] = {SOLAR_CELL_1, SOLAR_CELL_2, SOLAR_CELL_3, SOLAR_CELL_4, 
                ENABLE_BUCK_BOOST_CONVERTER, BYPASS_MPPT, CAPACITOR_3, CAPACITOR_4, 
                CONSTANT_LOAD, NIGHT_LOAD, HEAVY_LOAD};
  
  const char* namen[] = {"Solar 1", "Solar 2", "Solar 3", "Solar 4", 
                         "Buck/Boost", "MPPT", "Cap 3", "Cap 4", 
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