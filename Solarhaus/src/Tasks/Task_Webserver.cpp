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
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(200))) {
            localSystemState = sysState;
            xSemaphoreGive(dataMutex);
            }

            // NEU: BatCount direkt aus der auf der Webseite eingestellten Variablen auslesen
            batCount = localSystemState.batteryActiveCount;
            
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
        // --- NEU: Fortschritt und fiktive Uhrzeit berechnen ---
        float progress = 0.0;
        int simHour = 0;
        int simMinute = 0;

        if (localSystemState.isSimActive) {
            unsigned long currentMillis = millis();
            unsigned long durationMillis = (localSystemState.isDayPhase ? localSystemState.dayDurationSec : localSystemState.nightDurationSec) * 1000UL;
            
            if (durationMillis > 0) {
                unsigned long elapsed = currentMillis - localSystemState.simTimerStart;
                progress = (float)elapsed / (float)durationMillis;
                if (progress > 1.0) progress = 1.0;
            }

            // Fiktive 24h-Zeit berechnen
            float simTimeFloat = 0;
            if (localSystemState.isDayPhase) {
                simTimeFloat = 6.0 + (progress * 12.0); // Tag: 06:00 bis 18:00
            } else {
                simTimeFloat = 18.0 + (progress * 12.0); // Nacht: 18:00 bis 06:00
                if (simTimeFloat >= 24.0) simTimeFloat -= 24.0; // Über Mitternacht
            }
            simHour = (int)simTimeFloat;
            simMinute = (int)((simTimeFloat - simHour) * 60);
        }

        // JSON zusammenbauen
        // Allgemeine Statuswerte
        json += "\"bus_voltage\":" + String(localSystemState.busVoltage[1], 3) + ",";
        json += "\"adc_battery_voltage\":" + String(localSystemState.adcBatteryVoltage, 2) + ",";
        json += "\"adc_battery_percentage\":" + String(localSystemState.adcBatteryPercentage, 1) + ",";
        json += "\"solar_count\":" + String(solarCount) + ",";
        json += "\"battery_count\":" + String(batCount) + ",";
        json += "\"sim_active\":" + String(localSystemState.isSimActive ? "true" : "false") + ",";
        json += "\"is_day\":" + String(localSystemState.isDayPhase ? "true" : "false") + ",";
        json += "\"cur_cycle\":" + String(localSystemState.currentCycle) + ",";
        json += "\"max_cycles\":" + String(localSystemState.targetCycles)+ ","; 
        json += "\"sim_progress\":" + String(progress, 3) + ",";
        json += "\"sim_hour\":" + String(simHour) + ",";
        json += "\"sim_minute\":" + String(simMinute) + ",";
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
                    // Wenn die letzte Batterie ausgeschaltet wird, schalte auch alle Solarzellen und Lasten aus
                    if (newCount == 0) {
                        sysState.solarActiveCount = 0;
                        sysState.constantLoadOn = false;
                        sysState.nightLoadOn = false;
                        sysState.heavyLoadOn = false;
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
                // --- NEU: Verhindere das Einschalten, wenn keine Batterie da ist ---
                if (sysState.batteryActiveCount == 0) {
                    // Zur Sicherheit alles auf false zwingen
                    sysState.constantLoadOn = false;
                    sysState.nightLoadOn = false;
                    sysState.heavyLoadOn = false;
                } else {
                    // Normales Umschalten erlauben
                    if (load == "const") {
                        sysState.constantLoadOn = !sysState.constantLoadOn;
                    } else if (load == "night") { 
                        sysState.nightLoadOn = !sysState.nightLoadOn;
                    } else if (load == "heavy") { 
                        sysState.heavyLoadOn = !sysState.heavyLoadOn;
                    }
                }
                // -------------------------------------------------------------------
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
                    // --- NEU: Zeitpläne auslesen ---
                    // Constant Load
                    if(request->hasParam("cA")) sysState.schedConstActive = (request->getParam("cA")->value() == "true");
                    if(request->hasParam("cS")) { String s = request->getParam("cS")->value(); sysState.schedConstStartH = s.substring(0, 2).toInt(); sysState.schedConstStartM = s.substring(3, 5).toInt(); }
                    if(request->hasParam("cE")) { String s = request->getParam("cE")->value(); sysState.schedConstEndH = s.substring(0, 2).toInt(); sysState.schedConstEndM = s.substring(3, 5).toInt(); }
                    
                    // Night Load
                    if(request->hasParam("nA")) sysState.schedNightActive = (request->getParam("nA")->value() == "true");
                    if(request->hasParam("nS")) { String s = request->getParam("nS")->value(); sysState.schedNightStartH = s.substring(0, 2).toInt(); sysState.schedNightStartM = s.substring(3, 5).toInt(); }
                    if(request->hasParam("nE")) { String s = request->getParam("nE")->value(); sysState.schedNightEndH = s.substring(0, 2).toInt(); sysState.schedNightEndM = s.substring(3, 5).toInt(); }
                    
                    // Heavy Load
                    if(request->hasParam("hA")) sysState.schedHeavyActive = (request->getParam("hA")->value() == "true");
                    if(request->hasParam("hS")) { String s = request->getParam("hS")->value(); sysState.schedHeavyStartH = s.substring(0, 2).toInt(); sysState.schedHeavyStartM = s.substring(3, 5).toInt(); }
                    if(request->hasParam("hE")) { String s = request->getParam("hE")->value(); sysState.schedHeavyEndH = s.substring(0, 2).toInt(); sysState.schedHeavyEndM = s.substring(3, 5).toInt(); }

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

    server.on("/toggle_mppt_bypass", HTTP_GET, [](AsyncWebServerRequest *request){
        if(xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Status umkehren
            sysState.mpptBypassOn = !sysState.mpptBypassOn; 
            
            // Neuen Status als String zurücksenden ("1" für an, "0" für aus)
            String responseStr = sysState.mpptBypassOn ? "1" : "0";
            xSemaphoreGive(dataMutex);
            
            request->send(200, "text/plain", responseStr);
        } else {
            request->send(500, "text/plain", "Mutex Error");
        }
    });

    server.begin();

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}