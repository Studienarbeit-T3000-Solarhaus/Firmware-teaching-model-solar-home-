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

// --- NEU: Tag/Nacht Simulation Variablen ---
bool isDayMode = true;              // Startet mit Tag
unsigned long lastCycleChange = 0;  // Zeitstempel des letzten Wechsels
const unsigned long CYCLE_DURATION = 120000; // 2 Minuten pro Phase (in ms)
// -------------------------------------------

String processor(const String& var) {
    float currentVoltage = 0.0;
    xQueuePeek(voltageQueue, &currentVoltage, 0);
    if(var == "VOLTAGE") return String(currentVoltage, 3);
    return String();
}

void WebServerTask(void *parameter) {

    WiFi.softAP(ssid, password);
    Serial.printf("Webserver IP: %s\n", WiFi.softAPIP().toString().c_str());
    
    // Initialisiere Timer
    lastCycleChange = millis();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html, processor);
    });

    server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("type") && request->hasParam("idx") && request->hasParam("state")) {
            String type = request->arg("type");
            int idx = request->arg("idx").toInt();
            bool state = (request->arg("state").toInt() == 1);
            
            // --- NEU: Verhindere Solar-Einschalten bei Nacht ---
            if (type == "solar" && !isDayMode && state == true) {
                 Serial.println("Solar-Aktivierung blockiert (Nachtmodus)");
                 request->send(200, "text/plain", "BLOCKED_NIGHT");
                 return;
            }
            // --------------------------------------------------

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
        float ina_volts = 0.0; float ina_mA = 0.0; float ina_mW = 0.0;
        
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            ina_volts = ina219.getBusVoltage_V();
            ina_mA    = ina219.getCurrent_mA();
            ina_mW    = ina219.getPower_mW();
            xSemaphoreGive(i2cMutex); 
        }

        String json = "{";
        json += "\"voltage\":\"" + String(v_analog, 3) + "\",";
        json += "\"ina_v\":\""   + String(ina_volts, 2) + "\",";
        json += "\"ina_ma\":\""  + String(ina_mA, 1) + "\",";
        json += "\"ina_mw\":\""  + String(ina_mW, 1) + "\",";

        // --- NEU: Simulations-Status senden ---
        long timeLeft = (CYCLE_DURATION - (millis() - lastCycleChange)) / 1000;
        if (timeLeft < 0) timeLeft = 0;
        json += "\"isDay\":" + String(isDayMode ? "true" : "false") + ",";
        json += "\"timeLeft\":" + String(timeLeft) + ",";
        // --------------------------------------

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

    // --- Hauptschleife für Tag/Nacht Logik ---
    for (;;) {
        unsigned long now = millis();
        
        // Prüfen, ob Zeit abgelaufen ist
        if (now - lastCycleChange >= CYCLE_DURATION) {
            lastCycleChange = now;
            isDayMode = !isDayMode; // Wechseln
            
            Serial.printf("Modus Wechsel: Jetzt ist %s\n", isDayMode ? "TAG" : "NACHT");

            // Wenn Nacht wird: Alles Solar abschalten
            if (!isDayMode) {
                if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    for(int i=0; i<4; i++) {
                        // Software-Status aktualisieren
                        SolarMosfets[i] = false;
                        // Hardware abschalten
                        mcp.digitalWrite(SOLAR_PINS[i], LOW);
                    }
                    xSemaphoreGive(i2cMutex);
                    Serial.println("Nachtmodus: Alle Solarmodule getrennt.");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500)); // Logik alle 0.5s prüfen
    }
}

void startWebTask() {
    xTaskCreate(WebServerTask, "WebTask", 8192, NULL, 2, NULL);
}