#include "Webserver.h"
#include "Website.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "VoltageReader\VoltageReader.h" // Um auf voltageQueue zuzugreifen
#include "IMUSensor\IMUSensor.h"  // Um auf sensorQueue zuzugreifen

// Externe Variablen deklarieren (aus main.cpp übernommen)
AsyncWebServer server(80);
const char *ssid = "ESP32-C3_Sensor";
const char *password = "Solarhaus";

const int GPIO_PIN_7 = 21;
const int GPIO_PIN_6 = 20;
volatile bool pin7State = LOW;
volatile bool pin6State = LOW;

// Processor für Platzhalter im HTML
String processor(const String& var) {
    float currentVoltage = 0.0;
    xQueuePeek(voltageQueue, &currentVoltage, 0);
    
    if(var == "VOLTAGE") return String(currentVoltage, 3);
    if(var == "GPIO_7_STATE") return pin7State ? "AN" : "AUS";
    if(var == "GPIO_6_STATE") return pin6State ? "AN" : "AUS";
    return String();
}

void WebServerTask(void *parameter) {
    pinMode(GPIO_PIN_7, OUTPUT);
    pinMode(GPIO_PIN_6, OUTPUT);
    digitalWrite(GPIO_PIN_7, pin7State);
    digitalWrite(GPIO_PIN_6, pin6State);

    WiFi.softAP(ssid, password);
    Serial.printf("Webserver auf IP: %s\n", WiFi.softAPIP().toString().c_str());

    // Route: Index
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html, processor);
    });

    // Route: Steuerung (Set Pins)
    server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("pin7state") && request->hasParam("pin6state")) {
            pin7State = (request->arg("pin7state").toInt() == 1);
            pin6State = (request->arg("pin6state").toInt() == 1);
            digitalWrite(GPIO_PIN_7, pin7State);
            digitalWrite(GPIO_PIN_6, pin6State);
            Serial.printf("Pin %d auf Zustand %d gesetzt\n", GPIO_PIN_7, pin7State);
            Serial.printf("Pin %d auf Zustand %d gesetzt\n", GPIO_PIN_6, pin6State);
        } else if (request->hasParam("pin") && request->hasParam("state")) {
            int pin = request->arg("pin").toInt();
            int state = (request->arg("state").toInt() == 1);
            if (pin == 7) { pin7State = state; digitalWrite(7, state); }
            else if (pin == 6) { pin6State = state; digitalWrite(6, state); }
            Serial.printf("Pin %d auf Zustand %d gesetzt\n", pin, state);
        }
        request->send(200, "text/plain", "OK");
    });

    // Route: Status JSON für AJAX Updates
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        float currentVoltage = 0.0;
        SensorData currentSensorData;
        
        xQueuePeek(voltageQueue, &currentVoltage, 0);
        xQueuePeek(sensorQueue, &currentSensorData, 0);

        String json = "{\"voltage\":\"" + String(currentVoltage, 3) + 
                      "\",\"pin7State\":\"" + (pin7State ? "AN" : "AUS") + 
                      "\",\"pin6State\":\"" + (pin6State ? "AN" : "AUS") + 
                      "\",\"accelX\":\"" + String(currentSensorData.accelX, 2) + 
                      "\",\"accelY\":\"" + String(currentSensorData.accelY, 2) + 
                      "\",\"accelZ\":\"" + String(currentSensorData.accelZ, 2) + 
                      "\",\"temp\":\"" + String(currentSensorData.temperature, 1) + "\"}";
        request->send(200, "application/json", json);
    });

    server.begin();

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(100)); // Geringe CPU-Last im Loop
    }
}

void startWebTask() {
    xTaskCreate(WebServerTask, "WebTask", 8192, NULL, 2, NULL);
}