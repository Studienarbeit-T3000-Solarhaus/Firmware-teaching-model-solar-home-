#include <Arduino.h>
#include "VoltageReader\VoltageReader.h"
#include "DisplayLED\DisplayLED.h"
#include "Webserver\Webserver.h"
#include "DisplayLED\LedStrip.hpp"
#include "TestGPIOExtension.h"


Adafruit_MCP23X17 mcp;
SemaphoreHandle_t i2cMutex;
Adafruit_INA219 ina219;

void setup() {
    Serial.begin(115200);
    Wire.begin(D4, D5, 1700000); // SDA, SCL Pins und 1.7MHz Geschwindigkeit für MCP23017
    if (!mcp.begin_I2C(0x20)) {
    Serial.println("MCP23017 nicht gefunden! Checke die Verkabelung.");
    while (1);
  }else{
    Serial.println("MCP23017 erfolgreich initialisiert.");
  }
  if (!ina219.begin()) {
        Serial.println("INA219 nicht gefunden!");
    }else {
        ina219.setCalibration_16V_400mA();
        Serial.println("INA219 erfolgreich gestartet.");
    }
  mcp.pinMode(0, OUTPUT); // GPA0 als Ausgang
    i2cMutex = xSemaphoreCreateMutex();
    delay(2000);

    // Starte alle Module
    startVoltageTask();
    //startLEDTask();
    startWebTask();
    //startLedStripTask();
    startTestGPIOExtensionTask();
}

void loop() {
    // FreeRTOS übernimmt, loop bleibt leer
    vTaskDelay(pdMS_TO_TICKS(1000));
}