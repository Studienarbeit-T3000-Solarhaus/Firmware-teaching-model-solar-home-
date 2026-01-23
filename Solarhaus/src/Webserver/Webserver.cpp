#include "Webserver.h"
#include "Website.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "VoltageReader\VoltageReader.h"

AsyncWebServer server(80);
const char *ssid = "ESP32-C3_Sensor";
const char *password = "Solarhaus";

// --- KONFIGURATION DER PINS ---
// Bitte hier deine tatsächlichen GPIO-Nummern eintragen!
//const int SOLAR_PINS[4] = {2, 3, 4, 5};   // Beispiel-Pins
//const int AKKU_PINS[4]  = {6, 7, 8, 9};   // Beispiel-Pins
//const int LOAD_PINS[2]  = {10, 1};        // Beispiel-Pins

// Status-Speicher
bool SolarMosfets[4] = {false};
bool BatteryMosfets[4]  = {false};
bool LoadMosfets[2]  = {false};

// Hilfsfunktion zum Initialisieren
void initPins() {
    //for(int i=0; i<4; i++) { pinMode(SOLAR_PINS[i], OUTPUT); digitalWrite(SOLAR_PINS[i], LOW); }
    //for(int i=0; i<4; i++) { pinMode(AKKU_PINS[i], OUTPUT); digitalWrite(AKKU_PINS[i], LOW); }
    //for(int i=0; i<2; i++) { pinMode(LOAD_PINS[i], OUTPUT); digitalWrite(LOAD_PINS[i], LOW); }
}

String processor(const String& var) {
    // Einfache Platzhalter, falls nötig. 
    // Die UI aktualisiert sich aber hauptsächlich über JSON /status
    float currentVoltage = 0.0;
    xQueuePeek(voltageQueue, &currentVoltage, 0);
    if(var == "VOLTAGE") return String(currentVoltage, 3);
    return String();
}

void WebServerTask(void *parameter) {
    initPins(); // Pins auf Output setzen

    WiFi.softAP(ssid, password);
    Serial.printf("Webserver IP: %s\n", WiFi.softAPIP().toString().c_str());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html, processor);
    });

    // Neue universelle Route: /set?type=solar&idx=0&state=1
    server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("type") && request->hasParam("idx") && request->hasParam("state")) {
            String type = request->arg("type");
            int idx = request->arg("idx").toInt();
            bool state = (request->arg("state").toInt() == 1);

            if (type == "solar" && idx >= 0 && idx < 4) {
                SolarMosfets[idx] = state;
                //digitalWrite(SOLAR_PINS[idx], state);
            } 
            else if (type == "akku" && idx >= 0 && idx < 4) {
                BatteryMosfets[idx] = state;
                //digitalWrite(AKKU_PINS[idx], state);
            }
            else if (type == "load" && idx >= 0 && idx < 2) {
                LoadMosfets[idx] = state;
                //digitalWrite(LOAD_PINS[idx], state);
            }
            Serial.printf("SET %s [%d] -> %d\n", type.c_str(), idx, state);
        }
        request->send(200, "text/plain", "OK");
    });

    // JSON Status Update
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        float v = 0.0;
        xQueuePeek(voltageQueue, &v, 0);

        // JSON manuell bauen (ArduinoJson wäre schöner, aber so spart man Speicher)
        String json = "{";
        json += "\"voltage\":\"" + String(v, 3) + "\",";
        
        // Arrays in JSON einfügen: "solar":[0,1,0,0], ...
        json += "\"solar\":[";
        for(int i=0; i<4; i++) json += String(SolarMosfets[i]) + (i<3?",":"");
        json += "],";

        json += "\"akku\":[";
        for(int i=0; i<4; i++) json += String(BatteryMosfets[i]) + (i<3?",":"");
        json += "],";

        json += "\"load\":[";
        for(int i=0; i<2; i++) json += String(LoadMosfets[i]) + (i<1?",":"");
        json += "]";
        json += "}";


        request->send(200, "application/json", json);
    });

    server.begin();

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void startWebTask() {
    xTaskCreate(WebServerTask, "WebTask", 8192, NULL, 2, NULL);
}