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
const char *password = "Solarhaus"; 

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

// IMU
#include <Wire.h>
#include "GY521.h"

// I2C Pins (Standard für ESP32-C3 ist oft SDA=8, SCL=10, 
// aber prüfen Sie Ihr spezifisches Board. Wir verwenden die Standard-Pins.
#define SDA_PIN 3 
#define SCL_PIN 4

GY521 sensor(0x68); // Erstellt ein GY521-Objekt mit der Standard-I2C-Adresse

// Queue für die Sensordaten: Beschleunigung (X, Y, Z) und Temperatur (T)
struct SensorData {
    float accelX;
    float accelY;
    float accelZ;
    float temperature;
};
QueueHandle_t sensorQueue;
// --- FREERTOS TASKS KONFIGURATION ---
const int READ_TASK_PRIORITY = 2; 
const int LED_TASK_PRIORITY = 1;  
const int WEB_TASK_PRIORITY = 2; 
const int SENSOR_TASK_PRIORITY = 2; 

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

    <h3>MPU-6050 Beschleunigung (g) & Temp (&deg;C)</h3>
    <p>
      X: <span id="accelX">0.00</span> | 
      Y: <span id="accelY">0.00</span> | 
      Z: <span id="accelZ">0.00</span> 
    </p>
    <p>Temperatur: <span id="temp">0.0</span> &deg;C</p>
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

          // NEUE UPDATES FÜR MPU-6050
        document.getElementById('accelX').innerHTML = json.accelX;
        document.getElementById('accelY').innerHTML = json.accelY;
        document.getElementById('accelZ').innerHTML = json.accelZ;
        document.getElementById('temp').innerHTML = json.temp;
        }
      };
      xhr.open("GET", "/status", true);
      xhr.send();
    }, 50); // Alle 10ms aktualisieren
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
    pixels.setBrightness(30); 
    pixels.clear(); 
    pixels.show();
    
    float receivedVoltage = 0.0;
    
    for (;;) { 
        
        // Liest den Wert mit Peek, ohne ihn zu entfernen
        if (xQueuePeek(voltageQueue, &receivedVoltage, 0) == pdPASS) {
            
            // 1. Normalisierung der Spannung (0.0 bis 1.0)
            float normalizedValue = (receivedVoltage - MIN_V) / (MAX_V - MIN_V);
            normalizedValue = constrain(normalizedValue, 0.0, 1.0);
            
            // 2. Skalierung des Wertes für die 3 LEDs (0.0 bis 3.0)
            float scaledValue = normalizedValue * NUM_PIXELS; // z.B. 0.0 bis 3.0

            for (int i = 0; i < NUM_PIXELS; i++) {
                uint32_t color = 0;
                
                // Schwellenwert für das Einschalten der LED i
                if (scaledValue > (float)i) {
                    
                    // 3. Berechnung der Helligkeit (Brightness) und Farbe 
                    //    innerhalb des aktuellen LED-Segments.
                    
                    // Der 'brightnessFactor' geht von 0.0 bis 1.0 für das Segment,
                    // das gerade leuchtet oder gerade aufgefüllt wird.
                    float brightnessFactor = scaledValue - (float)i;
                    brightnessFactor = constrain(brightnessFactor, 0.0, 1.0);
                    
                    // Beispiel: 
                    // i=0 (LED 1): Segment 0.0 - 1.0
                    // i=1 (LED 2): Segment 1.0 - 2.0
                    // i=2 (LED 3): Segment 2.0 - 3.0

                    // **Farbverlauf:** Blau (niedrig) -> Grün (mittel) -> Rot (hoch)
                    // Die Farbe wird basierend auf dem globalen normalizedValue bestimmt,
                    // damit alle aktiven LEDs die gleiche Farbe haben.
                    
                    int blue = 0;
                    int green = 0;
                    int red = 0;

                    if (normalizedValue < 0.5) {
                        // Von Blau nach Grün (0.0 bis 0.5)
                        float mapValue = normalizedValue * 2.0; // 0.0 -> 1.0
                        blue = (int)((1.0 - mapValue) * 255 * brightnessFactor); 
                        green = (int)(mapValue * 255 * brightnessFactor);
                    } else {
                        // Von Grün nach Rot (0.5 bis 1.0)
                        float mapValue = (normalizedValue - 0.5) * 2.0; // 0.0 -> 1.0
                        green = (int)((1.0 - mapValue) * 255 * brightnessFactor); 
                        red = (int)(mapValue * 255 * brightnessFactor);
                    }
                    
                    // Helligkeit fixieren und Farbe setzen
                    pixels.setBrightness(255); // Oder eine konstante Helligkeit für bessere Sichtbarkeit
                    color = pixels.Color(red, green, blue);
                } 
                // Wenn scaledValue <= i, ist die LED aus (Farbe = 0)
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
    SensorData currentSensorData;

    // Liest den neuesten Spannungswert
    xQueuePeek(voltageQueue, &currentVoltage, 0); 
    
    // NEU: Liest die neuesten Sensordaten
    xQueuePeek(sensorQueue, &currentSensorData, 0); 
    
    String json = "{\"voltage\":\"" + String(currentVoltage, 3) + 
                  "\",\"pin7State\":\"" + (pin7State ? "AN" : "AUS") + 
                  "\",\"pin6State\":\"" + (pin6State ? "AN" : "AUS") + 
                  
                  // NEUE SENSOREINTRÄGE
                  "\",\"accelX\":\"" + String(currentSensorData.accelX, 2) + 
                  "\",\"accelY\":\"" + String(currentSensorData.accelY, 2) + 
                  "\",\"accelZ\":\"" + String(currentSensorData.accelZ, 2) + 
                  "\",\"temp\":\"" + String(currentSensorData.temperature, 1) + 
                  "\"}";
                      
    request->send(200, "application/json", json);
});

  server.begin();

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// -------------------------------------------------------------------
// TASK 4: GY-521 Beschleunigungssensor auslesen
// -------------------------------------------------------------------
void ReadSensorTask(void *parameter) {
    
    // I2C initialisieren
    Wire.begin(SDA_PIN, SCL_PIN); 
    sensor.begin(); 

    // Kalibrierung (wichtig für genaue Messungen)
    Serial.println("SENSOR_TASK: Starte Kalibrierung des GY-521...");
    sensor.calibrate(100); 
    Serial.println("SENSOR_TASK: Kalibrierung abgeschlossen.");
    
    // Initialisiere Queue mit 0-Werten
    SensorData initialData = {0.0, 0.0, 0.0, 0.0};
    xQueueOverwrite(sensorQueue, &initialData);

    for (;;) { 
        
        // Liest alle Sensordaten
        sensor.read();

        // Speichert die g-Werte (Beschleunigung) und Temperatur
        SensorData currentData;
        currentData.accelX = sensor.getAccelX();
        currentData.accelY = sensor.getAccelY();
        currentData.accelZ = sensor.getAccelZ();
        currentData.temperature = sensor.getTemperature();

        Serial.printf("SENSOR_TASK: X: %.2f | Y: %.2f | Z: %.2f | T: %.1f\n", 
                      currentData.accelX, currentData.accelY, currentData.accelZ, currentData.temperature);

        // Sendet die Daten an die Queue (Overwrite-Modus)
        xQueueOverwrite(sensorQueue, &currentData);
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Leseintervall: 100ms
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

  // NEU: Erstelle die Sensor-Queue
    sensorQueue = xQueueCreate(1, sizeof(SensorData));

    if (voltageQueue == NULL || sensorQueue == NULL) {
        Serial.println("Fehler: Konnte eine FreeRTOS Queue nicht erstellen.");
        while(1); 
    }

  // Erstelle die drei Tasks
  xTaskCreate(ReadVoltageTask, "ReadVoltageTask", 4096, NULL, READ_TASK_PRIORITY, NULL);
  xTaskCreate(NeoPixelTask, "NeoPixelTask", 4096, NULL, LED_TASK_PRIORITY, NULL);
  xTaskCreate(WebServerTask, "WebServerTask", 8192, NULL, WEB_TASK_PRIORITY, NULL);
  xTaskCreate(ReadSensorTask, "ReadSensorTask", 4096, NULL, SENSOR_TASK_PRIORITY, NULL);
}

void loop() {
  // FreeRTOS übernimmt die Kontrolle.
  vTaskDelay(pdMS_TO_TICKS(1));
}