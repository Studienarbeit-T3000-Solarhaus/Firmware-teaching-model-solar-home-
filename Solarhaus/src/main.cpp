#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// --- KONFIGURATIONEN ---
// WLAN Access Point
const char *ssid = "ESP32-C3_Sensor";
const char *password = "start1234"; 

// Hardware Pins
const int ANALOG_PIN = 2; // ADC-Eingang für die Spannung
const int NEOPIXEL_PIN = 9; // GPIO-Pin für die NeoPixel-Datenleitung
const int NUM_PIXELS = 3; 
const int GPIO_PIN_7 = 7; // Web-gesteuerter Pin
const int GPIO_PIN_6 = 6; // Web-gesteuerter Pin

// Mess- und Farbskala
const int NUM_READINGS = 100;
const float MIN_V = 0.3; // Untergrenze der Spannung für die Skala
const float MAX_V = 0.9; // Obergrenze der Spannung für die Skala

// --- FREERTOS QUEUE & GLOBALE VARIABLEN ---
// Queue speichert nur den neuesten Float-Wert
QueueHandle_t voltageQueue; 
AsyncWebServer server(80);
volatile bool pin7State = LOW; 
volatile bool pin6State = LOW; 

// --- FREERTOS TASKS KONFIGURATION ---
const int READ_TASK_PRIORITY = 2; 
const int LED_TASK_PRIORITY = 1;  
const int WEB_TASK_PRIORITY = 2; 


// -------------------------------------------------------------------
// HTML-CODE FÜR DIE WEBSEITE
// -------------------------------------------------------------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP32-C3 Steuerung & Sensor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; margin:auto; padding-top: 30px;}
    .container { max-width: 400px; margin: 0 auto; padding: 20px; border: 1px solid #ccc; border-radius: 10px; }
    .btn { padding: 10px 20px; font-size: 16px; margin: 5px; cursor: pointer; border: none; border-radius: 5px; color: white; }
    .btn-on { background-color: #4CAF50; }
    .btn-off { background-color: #f44336; }
    .voltage-display { font-size: 24px; color: #007bff; margin-top: 20px; font-weight: bold; }
  </style>
</head>
<body>
  <div class="container">
    <h2>ESP32-C3 Steuerung</h2>
    <div class="voltage-display">Spannung: <span id="voltage">%VOLTAGE%</span> V</div>
    <hr>
    
    <h3>Beide LEDs (7 & 6) steuern</h3>
    <button class="btn btn-on" onclick="toggleBoth(1)">ALLE AN</button>
    <button class="btn btn-off" onclick="toggleBoth(0)">ALLE AUS</button>
    <hr>
    
    <h3>GPIO 7 Steuerung</h3>
    <p>Aktueller Status: <span id="gpio7_state">%GPIO_7_STATE%</span></p>
    <button class="btn btn-on" onclick="toggleGPIO(7, 1)">AN</button>
    <button class="btn btn-off" onclick="toggleGPIO(7, 0)">AUS</button>

    <h3>GPIO 6 Steuerung</h3>
    <p>Aktueller Status: <span id="gpio6_state">%GPIO_6_STATE%</p>
    <button class="btn btn-on" onclick="toggleGPIO(6, 1)">AN</button>
    <button class="btn btn-off" onclick="toggleGPIO(6, 0)">AUS</button>
  </div>
  
  <script>
    function toggleGPIO(pin, state) {
      // Steuerung für einzelne Pins bleibt
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/set?pin=" + pin + "&state=" + state, true);
      xhr.send();
    }

    function toggleBoth(state) {
      // Neue Funktion sendet Status für beide Pins in einer Anfrage
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/set?pin7state=" + state + "&pin6state=" + state, true);
      xhr.send();
    }
    
    // Funktion zur Aktualisierung der Daten vom Server (AJAX/Polling)
    setInterval(function() {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var json = JSON.parse(this.responseText);
          // Spannung und GPIO-Status aktualisieren
          document.getElementById('voltage').innerHTML = json.voltage;
          document.getElementById('gpio7_state').innerHTML = json.pin7State;
          document.getElementById('gpio6_state').innerHTML = json.pin6State;
        }
      };
      xhr.open("GET", "/status", true);
      xhr.send();
    }, 10); // Alle 10ms aktualisieren
  </script>
</body>
</html>
)rawliteral";


// Funktion zum Ersetzen von Platzhaltern im HTML (für das ERSTE Laden)
String processor(const String& var){
  float currentVoltage = 0.0;
  
  // Liest den Wert mit Peek, ohne ihn zu entfernen
  xQueuePeek(voltageQueue, &currentVoltage, 0); 
  
  if(var == "VOLTAGE"){
    return String(currentVoltage, 3);
  }
  if(var == "GPIO_7_STATE"){
    return pin7State ? "AN" : "AUS";
  }
  if(var == "GPIO_6_STATE"){
    return pin6State ? "AN" : "AUS";
  }
  return String();
}


// -------------------------------------------------------------------
// TASK 1: Spannung auslesen, mitteln, kalibrieren und senden (Producer)
// -------------------------------------------------------------------
void ReadVoltageTask(void *parameter) {
  
  // ADC Setup
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db); 

  for (;;) { 
    long sumOfMilliVolts = 0; 
    
    // Mittelwertbildung
    for (int i = 0; i < NUM_READINGS; i++) {
      sumOfMilliVolts += analogReadMilliVolts(ANALOG_PIN);
    }
    float averageMv = (float)sumOfMilliVolts / NUM_READINGS;
    float averageVoltage = averageMv / 1000.0;
    
    Serial.print("TASK_READ: ");
    Serial.print(averageVoltage, 3);
    Serial.println(" V");

    // Sendet den Wert per Overwrite; stellt sicher, dass immer der neueste Wert verfügbar ist.
    xQueueOverwrite(voltageQueue, &averageVoltage);

    vTaskDelay(pdMS_TO_TICKS(10)); 
  }
}

// -------------------------------------------------------------------
// TASK 2: NeoPixel LEDs steuern (Consumer/Reader)
// -------------------------------------------------------------------
void NeoPixelTask(void *parameter) {
  
  Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
  pixels.begin();
  pixels.setBrightness(100); 
  pixels.clear(); 
  pixels.show();
  
  float receivedVoltage = 0.0;
  
  for (;;) { 
    
    // Liest den Wert mit Peek, ohne ihn zu entfernen (WICHTIGE ÄNDERUNG)
    if (xQueuePeek(voltageQueue, &receivedVoltage, 0) == pdPASS) {
      
      float normalizedValue = (receivedVoltage - MIN_V) / (MAX_V - MIN_V);
      normalizedValue = constrain(normalizedValue, 0.0, 1.0);

      int brightness = (int)(normalizedValue * 255);
      pixels.setBrightness(brightness);

      int blue = (int)((1.0 - normalizedValue) * 255); 
      int green = (int)(normalizedValue * 255);       

      uint32_t color = pixels.Color(0, green, blue); 

      for (int i = 0; i < NUM_PIXELS; i++) {
        pixels.setPixelColor(i, color);
      }
      pixels.show();
    } 
    
    vTaskDelay(pdMS_TO_TICKS(10)); 
  }
}


// -------------------------------------------------------------------
// TASK 3: Soft AP und Webserver (Consumer/Reader)
// -------------------------------------------------------------------
void WebServerTask(void *parameter) {
  
  pinMode(GPIO_PIN_7, OUTPUT);
  pinMode(GPIO_PIN_6, OUTPUT);
  digitalWrite(GPIO_PIN_7, pin7State);
  digitalWrite(GPIO_PIN_6, pin6State);

  // Starte WiFi AP
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.printf("Webserver gestartet. IP Adresse: %s\n", IP.toString().c_str());

  // 1. Hauptseite
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // 2. Steuerung
  // 2. Steuerung
server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
    
    // --- NEUE LOGIK: Verarbeitung von 2 Parametern (Beide AN/AUS) ---
    if (request->hasParam("pin7state") && request->hasParam("pin6state")) {
        // Lese den Status für Pin 7
        int state7 = request->arg("pin7state").toInt();
        pin7State = (state7 == 1);
        digitalWrite(GPIO_PIN_7, pin7State);
        Serial.printf("WEB: GPIO 7 (via BOTH) gesetzt auf: %s\n", pin7State ? "AN" : "AUS");

        // Lese den Status für Pin 6
        int state6 = request->arg("pin6state").toInt();
        pin6State = (state6 == 1);
        digitalWrite(GPIO_PIN_6, pin6State);
        Serial.printf("WEB: GPIO 6 (via BOTH) gesetzt auf: %s\n", pin6State ? "AN" : "AUS");
        
    } 
    // --- ALTE LOGIK: Verarbeitung von 1 Parameter (Einzeln AN/AUS) ---
    else if (request->hasParam("pin") && request->hasParam("state")) {
        int pin = request->arg("pin").toInt();
        int state = request->arg("state").toInt();
        
        if (pin == 7) {
            pin7State = (state == 1);
            digitalWrite(GPIO_PIN_7, pin7State);
            Serial.printf("WEB: GPIO 7 gesetzt auf: %s\n", pin7State ? "AN" : "AUS");
        } else if (pin == 6) { // Verwende 'else if', da nur ein Pin pro Aufruf gesendet wird
            pin6State = (state == 1);
            digitalWrite(GPIO_PIN_6, pin6State);
            Serial.printf("WEB: GPIO 6 gesetzt auf: %s\n", pin6State ? "AN" : "AUS");
        }
    }
    
    request->send(200, "text/plain", "OK");
});

  // 3. Status-Route (für JavaScript-Updates)
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    float currentVoltage = 0.0;
    
    // Liest den neuesten Wert, ohne ihn zu entfernen
    xQueuePeek(voltageQueue, &currentVoltage, 0); 
    
    String json = "{\"voltage\":\"" + String(currentVoltage, 3) + 
                  "\",\"pin7State\":\"" + (pin7State ? "AN" : "AUS") + 
                  "\",\"pin6State\":\"" + (pin6State ? "AN" : "AUS") + "\"}";
                  
    request->send(200, "application/json", json);
  });

  server.begin();

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// -------------------------------------------------------------------
// Arduino Hauptfunktionen (Setup & Loop)
// -------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(2000); 

  // Erstelle die Queue mit Größe 1 (Overwritten-Modus)
  voltageQueue = xQueueCreate(1, sizeof(float));

  if (voltageQueue == NULL) {
    Serial.println("Fehler: Konnte die FreeRTOS Queue nicht erstellen.");
    while(1); 
  }

  // Erstelle die drei Tasks
  xTaskCreate(ReadVoltageTask, "ReadVoltageTask", 4096, NULL, READ_TASK_PRIORITY, NULL);
  xTaskCreate(NeoPixelTask, "NeoPixelTask", 4096, NULL, LED_TASK_PRIORITY, NULL);
  xTaskCreate(WebServerTask, "WebServerTask", 8192, NULL, WEB_TASK_PRIORITY, NULL);
}

void loop() {
  // FreeRTOS übernimmt die Kontrolle.
  vTaskDelay(pdMS_TO_TICKS(1));
}