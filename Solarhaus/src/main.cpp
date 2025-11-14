#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// --- WIFI KONFIGURATION ---
const char *ssid = "ESP32-C3_Sensor";
const char *password = "start1234"; 

// --- HARDWARE KONFIGURATION ---
const int ANALOG_PIN = 2; // ADC-Eingang für die Spannung
const int NEOPIXEL_PIN = 9; // GPIO-Pin für die NeoPixel-Datenleitung
const int NUM_PIXELS = 3; 

// Neue GPIOs für die Web-Steuerung
const int GPIO_PIN_7 = 7; 
const int GPIO_PIN_8 = 8; 

// --- FREERTOS QUEUE ---
QueueHandle_t voltageQueue; // Für die Kommunikation zwischen ReadVoltageTask und WebServerTask

// --- FREERTOS TASKS KONFIGURATION ---
const int NUM_READINGS = 100;
const int READ_TASK_PRIORITY = 2; 
const int LED_TASK_PRIORITY = 1;  
const int WEB_TASK_PRIORITY = 2; // Gleiche Priorität wie ReadTask

// --- WEBSERVER OBJEKT ---
AsyncWebServer server(80);

// --- GPIO STATUS SPEICHER ---
// Diese werden vom WebServerTask und ReadVoltageTask (über Queue) genutzt
volatile bool pin7State = LOW; 
volatile bool pin8State = LOW; 

// --- SPANNUNGSBEREICH (für NeoPixel und Web-Anzeige) ---
const float MIN_V = 0.3; 
const float MAX_V = 0.9; 

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
    .gpio-status { font-size: 20px; margin: 10px; }
    .voltage-display { font-size: 24px; color: #007bff; margin-top: 20px; font-weight: bold; }
  </style>
</head>
<body>
  <div class="container">
    <h2>ESP32-C3 Steuerung</h2>
    <div class="voltage-display">Spannung: <span id="voltage">%VOLTAGE%</span> V</div>
    <hr>
    
    <h3>GPIO 7 Steuerung</h3>
    <p>Aktueller Status: <span id="gpio7_state">%GPIO_7_STATE%</span></p>
    <button class="btn btn-on" onclick="toggleGPIO(7, 1)">AN</button>
    <button class="btn btn-off" onclick="toggleGPIO(7, 0)">AUS</button>

    <h3>GPIO 8 Steuerung</h3>
    <p>Aktueller Status: <span id="gpio8_state">%GPIO_8_STATE%</span></p>
    <button class="btn btn-on" onclick="toggleGPIO(8, 1)">AN</button>
    <button class="btn btn-off" onclick="toggleGPIO(8, 0)">AUS</button>
  </div>
  
  <script>
    function toggleGPIO(pin, state) {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/set?pin=" + pin + "&state=" + state, true);
      xhr.send();
    }
    
    // Funktion zur Aktualisierung der Daten vom Server
    setInterval(function() {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var json = JSON.parse(this.responseText);
          document.getElementById('voltage').innerHTML = json.voltage;
          document.getElementById('gpio7_state').innerHTML = json.pin7State;
          document.getElementById('gpio8_state').innerHTML = json.pin8State;
          
          // Aktualisiere Button-Status optisch (optional)
          // ...
        }
      };
      xhr.open("GET", "/status", true);
      xhr.send();
    }, 1000); // Alle 1000ms (1 Sekunde) aktualisieren
  </script>
</body>
</html>
)rawliteral";


// Funktion zum Ersetzen von Platzhaltern im HTML
String processor(const String& var){
  if(var == "VOLTAGE"){
    float currentVoltage = 0.0;
    // Versuche, den aktuellen Wert aus der Queue zu lesen
    if (xQueuePeek(voltageQueue, &currentVoltage, 0) == pdPASS) {
        return String(currentVoltage, 3);
    }
    return "N/A";
  }
  if(var == "GPIO_7_STATE"){
    return pin7State ? "AN" : "AUS";
  }
  if(var == "GPIO_8_STATE"){
    return pin8State ? "AN" : "AUS";
  }
  return String();
}


// -------------------------------------------------------------------
// Task 3: Soft AP und Webserver
// -------------------------------------------------------------------
void WebServerTask(void *parameter) {
  
  pinMode(GPIO_PIN_7, OUTPUT);
  pinMode(GPIO_PIN_8, OUTPUT);
  digitalWrite(GPIO_PIN_7, pin7State);
  digitalWrite(GPIO_PIN_8, pin8State);

  Serial.print("WebserverTask: Starte Soft AP...");
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.printf(" Fertig! IP Adresse: %s\n", IP.toString().c_str());

  // --- WEBSERVER ROUTEN DEFINITION ---
  
  // 1. Hauptseite
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // 2. Steuerung (Toggle) Route
  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("pin") && request->hasParam("state")) {
      int pin = request->arg("pin").toInt();
      int state = request->arg("state").toInt();
      
      if (pin == 7) {
        pin7State = (state == 1);
        digitalWrite(GPIO_PIN_7, pin7State);
        Serial.printf("WEB: GPIO 7 gesetzt auf: %s\n", pin7State ? "AN" : "AUS");
      } else if (pin == 8) {
        pin8State = (state == 1);
        digitalWrite(GPIO_PIN_8, pin8State);
        Serial.printf("WEB: GPIO 8 gesetzt auf: %s\n", pin8State ? "AN" : "AUS");
      }
    }
    // Nach der Steuerung zur Hauptseite umleiten oder einfach "OK" senden
    request->send(200, "text/plain", "OK");
  });

  // 3. Status-Route (für JavaScript-Updates)
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    float currentVoltage = 0.0;
    // Peek, um den Wert zu lesen, ohne ihn aus der Queue zu entfernen
    xQueuePeek(voltageQueue, &currentVoltage, 0); 
    
    String json = "{\"voltage\":\"" + String(currentVoltage, 3) + 
                  "\",\"pin7State\":\"" + (pin7State ? "AN" : "AUS") + 
                  "\",\"pin8State\":\"" + (pin8State ? "AN" : "AUS") + "\"}";
                  
    request->send(200, "application/json", json);
  });

  server.begin();

  for (;;) {
    // Der AsyncWebServer benötigt keine manuelle loop-Verarbeitung
    // Der Task muss nur am Leben erhalten werden. Eine kleine Verzögerung
    // gibt anderen Tasks Zeit.
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// -------------------------------------------------------------------
// Task 1: Spannung auslesen, mitteln, kalibrieren und senden (ANGEPASST)
// -------------------------------------------------------------------
void ReadVoltageTask(void *parameter) {
  
  //... (ADC Setup) ...
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db); 
  //...

  for (;;) { 
    long sumOfMilliVolts = 0; 
    
    for (int i = 0; i < NUM_READINGS; i++) {
      sumOfMilliVolts += analogReadMilliVolts(ANALOG_PIN);
    }
    float averageMv = (float)sumOfMilliVolts / NUM_READINGS;
    float averageVoltage = averageMv / 1000.0;
    
    Serial.print("TASK_READ: ");
    Serial.print(averageVoltage, 3);
    Serial.println(" V");

    // Senden des Spannungswerts an die Queue (für NeoPixelTask und WebServerTask)
    // Die Queue hat Größe 1, der alte Wert wird überschrieben
    if (xQueueOverwrite(voltageQueue, &averageVoltage) != pdPASS) {
        Serial.println("TASK_READ: Fehler beim Senden an die Queue.");
    }

    vTaskDelay(pdMS_TO_TICKS(500)); 
  }
}

// -------------------------------------------------------------------
// Task 2: NeoPixel LEDs steuern (UNVERÄNDERT)
// -------------------------------------------------------------------
void NeoPixelTask(void *parameter) {
  
  Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
  pixels.begin();
  pixels.setBrightness(100); 
  pixels.clear(); 
  pixels.show();
  
  float receivedVoltage = 0.0;
  
  for (;;) { 
    
    // Empfange den neuesten Spannungswert aus der Queue
    if (xQueueReceive(voltageQueue, &receivedVoltage, 10 / portTICK_PERIOD_MS) == pdPASS) {
      
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
// Arduino Hauptfunktionen
// -------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(2000); 

  // WICHTIG: Ändere die Queue auf xQueueOverwrite, da wir den aktuellsten Wert brauchen
  // Größe 1, um nur den neuesten Wert zu speichern.
  voltageQueue = xQueueCreate(1, sizeof(float));

  if (voltageQueue == NULL) {
    Serial.println("Fehler: Konnte die FreeRTOS Queue nicht erstellen.");
    while(1); 
  }

  // 1. ReadVoltageTask erstellen
  xTaskCreate(ReadVoltageTask, "ReadVoltageTask", 4096, NULL, READ_TASK_PRIORITY, NULL);

  // 2. NeoPixelTask erstellen
  xTaskCreate(NeoPixelTask, "NeoPixelTask", 4096, NULL, LED_TASK_PRIORITY, NULL);
  
  // 3. WebServerTask erstellen
  xTaskCreate(WebServerTask, "WebServerTask", 8192, NULL, WEB_TASK_PRIORITY, NULL); // Größerer Stack für Webserver

}

void loop() {
  // FreeRTOS übernimmt die Kontrolle
  vTaskDelay(pdMS_TO_TICKS(1)); 
}