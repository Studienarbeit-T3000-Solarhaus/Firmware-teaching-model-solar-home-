#ifndef WEBPAGES_H
#define WEBPAGES_H

#include <Arduino.h>



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

#endif // WEBPAGES_H