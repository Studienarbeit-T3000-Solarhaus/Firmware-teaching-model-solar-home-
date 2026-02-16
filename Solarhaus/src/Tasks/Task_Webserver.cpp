#include "tasks.hpp"
#include "shared_data.hpp"
#include "Config.hpp"
#include "DebugConfig.hpp"
#include "Webpage.hpp"
#include "PinDefinitions.hpp"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

SystemState localSystemState;

void Task_Webserver(void* pvParameters) {
    // 1. Setup WiFi
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    
    #ifdef DEBUG
    Serial.print("AP IP address: ");
    Serial.println(IP);
    #endif

    // 2. Define Routes

    // Serve HTML
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
    });

    // Status API (JSON)
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{";
        
        float v1=0, i1=0, v2=0, i2=0, v3=0, i3=0;
        int solarCount = 0;
        int batCount = 0;
        bool constLoadOn = false;
        bool nightLoadOn = false; 
        bool heavyLoadOn = false;

        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(200))) {
            // Read INA3221
            // Channel 0 (Solar), Channel 1 (Battery), Channel 2 (Load)
            v1 = CurrentSensor.getBusVoltage(0); i1 = CurrentSensor.getCurrentAmps(0) * 1000;
            v2 = CurrentSensor.getBusVoltage(1); i2 = CurrentSensor.getCurrentAmps(1) * 1000;
            v3 = CurrentSensor.getBusVoltage(2); i3 = CurrentSensor.getCurrentAmps(2) * 1000;

            // Read GPIO States
            for(int i=0; i<4; i++) if(GPIOExpander.digitalRead(SOLAR_CELL_1 + i)) solarCount++;
            for(int i=0; i<4; i++) if(GPIOExpander.digitalRead(CAPACITOR_1 + i)) batCount++;
            
            constLoadOn = GPIOExpander.digitalRead(CONSTANT_LOAD);
            nightLoadOn = GPIOExpander.digitalRead(NIGHT_LOAD);
            heavyLoadOn = GPIOExpander.digitalRead(HEAVY_LOAD);

            xSemaphoreGive(i2cMutex);
        }

        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(200))) {
            localSystemState = sysState;
            xSemaphoreGive(dataMutex);
        }

        // JSON zusammenbauen
        // Allgemeine Statuswerte
        json += "\"bus_voltage\":" + String(localSystemState.busVoltage[1], 3) + ",";
        json += "\"solar_count\":" + String(solarCount) + ",";
        json += "\"battery_count\":" + String(batCount) + ",";
        
        // Status der Verbraucher
        json += "\"const_on\":" + String(constLoadOn ? "true" : "false") + ",";
        json += "\"night_on\":" + String(nightLoadOn ? "true" : "false") + ",";
        json += "\"heavy_on\":" + String(heavyLoadOn ? "true" : "false") + ",";

        // Messwerte (Voltage, mA, mW)
        // Hinweis: ch1 = Solar (Ch0), ch2 = Battery (Ch1), ch3 = Load (Ch2)
        json += "\"ch1_v\":" + String(v1, 2) + ", \"ch1_ma\":" + String(i1, 0) + ", \"ch1_mw\":" + String(v1 * i1, 0) + ",";
        json += "\"ch2_v\":" + String(v2, 2) + ", \"ch2_ma\":" + String(i2, 0) + ", \"ch2_mw\":" + String(v2 * i2, 0) + ",";
        json += "\"ch3_v\":" + String(v3, 2) + ", \"ch3_ma\":" + String(i3, 0) + ", \"ch3_mw\":" + String(v3 * i3, 0);
        
        json += "}";
        request->send(200, "application/json", json);
    });

    server.on("/api/control", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("type") && request->hasParam("action")) {
            String type = request->getParam("type")->value();
            String action = request->getParam("action")->value();
            
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(200))) {
                int* countPtr = (type == "solar") ? &sysState.solarActiveCount : &sysState.batteryActiveCount;
                int newCount = *countPtr;

                if (action == "inc") newCount++;
                else if (action == "dec") newCount--;
                else if (action == "all_on") newCount = 4;
                else if (action == "all_off") newCount = 0;

                if (newCount > 4) newCount = 4;
                if (newCount < 0) newCount = 0;

                *countPtr = newCount;
                xSemaphoreGive(dataMutex);
            }
        }
        request->send(200, "text/plain", "OK");
    });

    // Toggle Loads
    server.on("/api/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("load")) {
            String load = request->getParam("load")->value();
            
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(200))) {
                if (load == "const") {
                    sysState.constantLoadOn = !sysState.constantLoadOn;
                } else if (load == "night") { 
                    sysState.nightLoadOn = !sysState.nightLoadOn;
                } else if (load == "heavy") { 
                    sysState.heavyLoadOn = !sysState.heavyLoadOn;
                }
                xSemaphoreGive(dataMutex);
            }
        }
        request->send(200, "text/plain", "OK");
    });

    server.begin();

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}