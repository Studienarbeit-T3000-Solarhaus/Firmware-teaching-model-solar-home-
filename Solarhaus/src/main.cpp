#include <Arduino.h>
#include "VoltageReader\VoltageReader.h"
#include "Webserver\Webserver.h"
#include "DisplayLED\LedStrip.hpp"
#include "Pins.h"


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
    mcp.pinMode(SOLAR_PIN_1, OUTPUT); // GPA0 als Ausgang
    mcp.pinMode(SOLAR_PIN_2, OUTPUT); // GPA1 als Ausgang
    mcp.pinMode(SOLAR_PIN_3, OUTPUT); // GPA2 als Ausgang
    mcp.pinMode(SOLAR_PIN_4, OUTPUT); // GPA3 als Ausgang
    mcp.pinMode(AKKU_PIN_1, OUTPUT); // GPA4 als Ausgang
    mcp.pinMode(AKKU_PIN_2, OUTPUT); // GPA5 als Ausgang
    mcp.pinMode(AKKU_PIN_3, OUTPUT); // GPA6 als Ausgang
    mcp.pinMode(AKKU_PIN_4, OUTPUT); // GPA7 als Ausgang
    mcp.pinMode(LOAD_PIN_1, OUTPUT); // GPB0 als Ausgang
    mcp.pinMode(LOAD_PIN_2, OUTPUT); // GPB1 als Ausgang

    i2cMutex = xSemaphoreCreateMutex();
    delay(2000);

    // Starte alle Module
    startVoltageTask();
    startWebTask();
    startLedStripTask();
}

void loop() {
    // FreeRTOS übernimmt, loop bleibt leer
    vTaskDelay(pdMS_TO_TICKS(1000));
}