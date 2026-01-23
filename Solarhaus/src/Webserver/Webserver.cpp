#include "Webserver.h"
#include "Website.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "VoltageReader\VoltageReader.h"
#include "Pins.h"

AsyncWebServer server(80);
const char *ssid = "ESP32-C3_Sensor";
const char *password = "Solarhaus";

const int SOLAR_PINS[4] = {SOLAR_PIN_1, SOLAR_PIN_2, SOLAR_PIN_3, SOLAR_PIN_4};
const int AKKU_PINS[4]  = {AKKU_PIN_1, AKKU_PIN_2, AKKU_PIN_3, AKKU_PIN_4};   
const int LOAD_PINS[2]  = {LOAD_PIN_1, LOAD_PIN_2};        

bool SolarMosfets[4] = {false};
bool BatteryMosfets[4]  = {false};
bool LoadMosfets[2]  = {false};

String processor(const String& var) {
    float currentVoltage = 0.0;
    xQueuePeek(voltageQueue, &currentVoltage, 0);
    if(var == "VOLTAGE") return String(currentVoltage, 3);
    return String();
}

void WebServerTask(void *parameter) {

    WiFi.softAP(ssid, password);
    Serial.printf("Webserver IP: %s\n", WiFi.softAPIP().toString().c_str());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html, processor);
    });

    server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("type") && request->hasParam("idx") && request->hasParam("state")) {
            String type = request->arg("type");
            int idx = request->arg("idx").toInt();
            bool state = (request->arg("state").toInt() == 1);
            
            int pinToSwitch = -1;

            if (type == "solar" && idx >= 0 && idx < 4) {
                SolarMosfets[idx] = state;
                pinToSwitch = SOLAR_PINS[idx];
            } 
            else if (type == "akku" && idx >= 0 && idx < 4) {
                BatteryMosfets[idx] = state;
                pinToSwitch = AKKU_PINS[idx];
            }
            else if (type == "load" && idx >= 0 && idx < 2) {
                LoadMosfets[idx] = state;
                pinToSwitch = LOAD_PINS[idx];
            }

            if (pinToSwitch != -1) {
                if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    mcp.digitalWrite(pinToSwitch, state ? HIGH : LOW);
                    xSemaphoreGive(i2cMutex);
                    Serial.printf("SET %s [%d] (Pin %d) -> %d\n", type.c_str(), idx, pinToSwitch, state);
                } else {
                    Serial.println("Fehler: I2C Mutex blockiert!");
                }
            }
        }
        request->send(200, "text/plain", "OK");
    });

    // JSON Status Update
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        float v_analog = 0.0;
        xQueuePeek(voltageQueue, &v_analog, 0);
        float ina_volts = 0.0;
        float ina_mA = 0.0;
        float ina_mW = 0.0;
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            ina_volts = ina219.getBusVoltage_V();
            ina_mA    = ina219.getCurrent_mA();
            ina_mW    = ina219.getPower_mW();
            xSemaphoreGive(i2cMutex); 
        } else {
            Serial.println("Warnung: Konnte INA219 nicht lesen (Mutex busy)");
        }

        // 3. JSON zusammenbauen
        String json = "{";
        
        // Werte in das JSON schreiben
        json += "\"voltage\":\"" + String(v_analog, 3) + "\","; // Alter Wert
        json += "\"ina_v\":\""   + String(ina_volts, 2) + "\",";
        json += "\"ina_ma\":\""  + String(ina_mA, 1) + "\",";
        json += "\"ina_mw\":\""  + String(ina_mW, 1) + "\",";

        // Arrays für die Buttons
        json += "\"solar\":[";
        for(int i=0; i<4; i++) json += String(SolarMosfets[i]) + (i<3?",":"");
        json += "],";

        json += "\"akku\":[";
        for(int i=0; i<4; i++) json += String(BatteryMosfets[i]) + (i<3?",":"");
        json += "],";

        json += "\"load\":[";
        for(int i=0; i<2; i++) json += String(LoadMosfets[i]) + (i<1?",":"");
        json += "]";
        
        json += "}"; // JSON schließen

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