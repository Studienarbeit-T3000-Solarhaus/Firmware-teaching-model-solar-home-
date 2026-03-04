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

        // --- NEU: Fortschritt berechnen ---
        float progress = 0.0;
        if (localSystemState.isSimActive) {
            unsigned long currentMillis = millis();
            unsigned long durationMillis = (localSystemState.isDayPhase ? localSystemState.dayDurationSec : localSystemState.nightDurationSec) * 1000UL;
            
            if (durationMillis > 0) {
                unsigned long elapsed = currentMillis - localSystemState.simTimerStart;
                progress = (float)elapsed / (float)durationMillis;
                if (progress > 1.0) progress = 1.0;
            }
        }

        // JSON zusammenbauen
        // Allgemeine Statuswerte
        json += "\"bus_voltage\":" + String(localSystemState.busVoltage[1], 3) + ",";
        json += "\"solar_count\":" + String(solarCount) + ",";
        json += "\"battery_count\":" + String(batCount) + ",";
        json += "\"sim_active\":" + String(localSystemState.isSimActive ? "true" : "false") + ",";
        json += "\"is_day\":" + String(localSystemState.isDayPhase ? "true" : "false") + ",";
        json += "\"cur_cycle\":" + String(localSystemState.currentCycle) + ",";
        json += "\"max_cycles\":" + String(localSystemState.targetCycles)+ ","; 
        json += "\"sim_progress\":" + String(progress, 3) + ",";
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
                if (sysState.isSimActive) {
                    xSemaphoreGive(dataMutex);
                    request->send(403, "text/plain", "Simulation Active");
                    return;
                }
                int* countPtr = (type == "solar") ? &sysState.solarActiveCount : &sysState.batteryActiveCount;
                int newCount = *countPtr;

                if (action == "inc") newCount++;
                else if (action == "dec") newCount--;
                else if (action == "all_on") newCount = 4;
                else if (action == "all_off") newCount = 0;

                if (newCount > 4) newCount = 4;
                if (newCount < 0) newCount = 0;

                // --- NEUE LOGIK: Abhängigkeit zwischen Solar und Batterie ---
                if (type == "solar") {
                    // Verhindere das Einschalten von Solar, wenn keine Batterie aktiv ist
                    if (sysState.batteryActiveCount == 0 && newCount > 0) {
                        newCount = 0; 
                    }
                } else if (type == "battery") {
                    // Wenn die letzte Batterie ausgeschaltet wird, schalte auch alle Solarzellen aus
                    if (newCount == 0 && sysState.solarActiveCount > 0) {
                        sysState.solarActiveCount = 0;
                    }
                }
                // ------------------------------------------------------------

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
                if (sysState.isSimActive) {
                    xSemaphoreGive(dataMutex);
                    request->send(403, "text/plain", "Simulation Active");
                    return;
                }
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

    // Simulation Control API
    server.on("/api/sim", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("active")) {
            bool setActive = (request->getParam("active")->value() == "true");
            
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(200))) {
                sysState.isSimActive = setActive;

                // Nur wenn wir Starten (active=true), lesen wir die Konfig-Werte ein
                if (setActive) {
                    if(request->hasParam("dayTime")) sysState.dayDurationSec = request->getParam("dayTime")->value().toInt();
                    if(request->hasParam("nightTime")) sysState.nightDurationSec = request->getParam("nightTime")->value().toInt();
                    
                    // --- NEU: Zyklen & Hardware Config direkt aus Params ---
                    if(request->hasParam("cycles")) sysState.targetCycles = request->getParam("cycles")->value().toInt();
                    if(request->hasParam("solar")) sysState.configSolarCount = request->getParam("solar")->value().toInt();
                    if(request->hasParam("bat")) sysState.configBatteryCount = request->getParam("bat")->value().toInt();
                    // -------------------------------------------------------
                    // NEU: Parameter für Constant Load auslesen
                    if(request->hasParam("nightConst")) sysState.configNightConstantLoad = (request->getParam("nightConst")->value() == "1");
                    // -------------------------------------------------------

                    // --- NEU: Log leeren beim Start ---
                    if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(200))) {
                        simulationLog.clear();
                        xSemaphoreGive(logMutex);
                    }
                    // ---------------------------------

                    // Start-Initialisierung
                    sysState.simTimerStart = millis();
                    sysState.isDayPhase = true;      // Start mit Tag
                    sysState.currentCycle = 1;       // Wir starten im 1. Zyklus
                    
                    // Hardware sofort setzen für den Start
                    sysState.solarActiveCount = sysState.configSolarCount;
                    sysState.batteryActiveCount = sysState.configBatteryCount;
                    sysState.nightLoadOn = false;    // Licht aus am Tag
                    sysState.constantLoadOn = false;
                }
                
                xSemaphoreGive(dataMutex);
            }
        }
        request->send(200, "text/plain", "OK");
    });


    // History API
    server.on("/api/history", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{\"data\":[";
        
        if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(500))) {
            for (size_t i = 0; i < simulationLog.size(); i++) {
                json += "[";
                // Um Platz zu sparen, senden wir Arrays statt Objekte: [vSolar, iSolar, vBat, iBat]
                json += String(simulationLog[i].vSolar, 2) + ",";
                json += String(simulationLog[i].iSolar, 0) + ",";
                json += String(simulationLog[i].vBat, 2) + ",";
                json += String(simulationLog[i].iBat, 0);
                json += "]";
                if (i < simulationLog.size() - 1) json += ",";
            }
            xSemaphoreGive(logMutex);
        }
        json += "]}";
        request->send(200, "application/json", json);
    });

    server.on("/setMode", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("auto")) {
            String autoVal = request->getParam("auto")->value();
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                sysState.mppt_auto_mode = (autoVal == "1");
                xSemaphoreGive(dataMutex);
            }
        }
        request->send(200, "text/plain", "OK");
    });

    // NEU: PWM Slider Wert setzen
    server.on("/setPWM", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("value")) {
            String pwmVal = request->getParam("value")->value();
            int val = pwmVal.toInt();
            if (val < 0) val = 0;
            if (val > 255) val = 255;
            
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                sysState.manual_pwm_value = val;
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