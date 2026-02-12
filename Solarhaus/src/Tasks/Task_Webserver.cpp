#include "tasks.hpp"
#include "shared_data.hpp"
#include "Config.hpp"
#include "Webpage.hpp"
#include "PinDefinitions.hpp"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
//#include <ArduinoJson.h> // Ensure you add ArduinoJson to lib_deps if not present, or construct JSON manually



AsyncWebServer server(80);

// Helper to count active bits in a range
int countActive(int startPin, int count) {
    int active = 0;
    // Note: We need to take Mutex, but since this is a helper called inside handlers
    // which already manage mutex or are quick, we handle mutex at the top level.
    // However, MCP23017 read requires I2C.
    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100))) {
        for(int i=0; i<count; i++) {
            if (GPIOExpander.digitalRead(startPin + i) == HIGH) {
                active++;
            }
        }
        xSemaphoreGive(i2cMutex);
    }
    return active;
}

// Helper to set pins incrementally
void setIncremental(int startPin, int maxCount, int targetCount) {
    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(200))) {
        for(int i=0; i<maxCount; i++) {
            if (i < targetCount) {
                GPIOExpander.digitalWrite(startPin + i, HIGH);
            } else {
                GPIOExpander.digitalWrite(startPin + i, LOW);
            }
        }
        xSemaphoreGive(i2cMutex);
    }
}

void Task_Webserver(void* pvParameters) {
    // 1. Setup WiFi (AP Mode for standalone demo, or STA to connect to router)
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
        
        // I2C Critical Section for Reading Sensors
        float v1=0, i1=0, v2=0, i2=0, v3=0, i3=0;
        int solarCount = 0;
        int batCount = 0;
        bool lightOn = false;
        bool machineOn = false;

        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(200))) {
            // Read INA3221
            v1 = CurrentSensor.getBusVoltage(0); i1 = CurrentSensor.getCurrentAmps(0) * 1000;
            v2 = CurrentSensor.getBusVoltage(1); i2 = CurrentSensor.getCurrentAmps(1) * 1000;
            v3 = CurrentSensor.getBusVoltage(2); i3 = CurrentSensor.getCurrentAmps(2) * 1000;

            // Read GPIO States
            // Solar: 0-3
            for(int i=0; i<4; i++) if(GPIOExpander.digitalRead(SOLAR_CELL_1 + i)) solarCount++;
            // Battery: 4-7
            for(int i=0; i<4; i++) if(GPIOExpander.digitalRead(CAPACITOR_1 + i)) batCount++;
            
            lightOn = GPIOExpander.digitalRead(CONSTANT_LOAD);
            machineOn = GPIOExpander.digitalRead(HEAVY_LOAD);

            xSemaphoreGive(i2cMutex);
        }

        json += "\"bus_voltage\":" + String(v1, 3) + ",";
        json += "\"solar_count\":" + String(solarCount) + ",";
        json += "\"battery_count\":" + String(batCount) + ",";
        json += "\"light_on\":" + String(lightOn ? "true" : "false") + ",";
        json += "\"machine_on\":" + String(machineOn ? "true" : "false") + ",";
        
        json += "\"ch1_v\":" + String(v1, 2) + ", \"ch1_ma\":" + String(i1, 0) + ",";
        json += "\"ch2_v\":" + String(v2, 2) + ", \"ch2_ma\":" + String(i2, 0) + ",";
        json += "\"ch3_v\":" + String(v3, 2) + ", \"ch3_ma\":" + String(i3, 0);
        
        json += "}";
        request->send(200, "application/json", json);
    });

    // Control API for Arrays (Solar/Battery)
    server.on("/api/control", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("type") && request->hasParam("action")) {
            String type = request->getParam("type")->value();
            String action = request->getParam("action")->value();
            
            int startPin = (type == "solar") ? SOLAR_CELL_1 : CAPACITOR_1;
            int currentCount = countActive(startPin, 4);
            int newCount = currentCount;

            if (action == "inc") newCount++;
            else if (action == "dec") newCount--;
            else if (action == "all_on") newCount = 4;
            else if (action == "all_off") newCount = 0;

            if (newCount > 4) newCount = 4;
            if (newCount < 0) newCount = 0;

            setIncremental(startPin, 4, newCount);
        }
        request->send(200, "text/plain", "OK");
    });

    // Toggle Loads
    server.on("/api/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("load")) {
            String load = request->getParam("load")->value();
            int pin = (load == "light") ? CONSTANT_LOAD : HEAVY_LOAD;
            
            if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(200))) {
                bool state = GPIOExpander.digitalRead(pin);
                GPIOExpander.digitalWrite(pin, !state);
                xSemaphoreGive(i2cMutex);
            }
        }
        request->send(200, "text/plain", "OK");
    });

    server.begin();

    while(1) {
        // Webserver is async, so this task can just monitor or yield
        // We can handle connection loss or other background logic here
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}