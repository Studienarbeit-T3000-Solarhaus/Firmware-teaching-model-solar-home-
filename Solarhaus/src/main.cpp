#include <Arduino.h>
#include "VoltageReader\VoltageReader.h"
#include "DisplayLED\DisplayLED.h"
#include "Webserver\Webserver.h"
#include "DisplayLED\LedStrip.hpp"

void setup() {
    Serial.begin(115200);
    delay(2000);

    // Starte alle Module
    startVoltageTask();
    //startLEDTask();
    startWebTask();
    startLedStripTask();
}

void loop() {
    // FreeRTOS übernimmt, loop bleibt leer
    vTaskDelay(pdMS_TO_TICKS(1000));
}