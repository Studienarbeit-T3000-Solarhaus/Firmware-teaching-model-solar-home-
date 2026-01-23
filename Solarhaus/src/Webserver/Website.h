#ifndef WEBPAGES_H
#define WEBPAGES_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Solarhaus Steuerung</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; margin: 0; padding: 20px; background-color: #f4f4f4; }
    .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
    h3 { margin-top: 20px; border-bottom: 2px solid #ddd; padding-bottom: 5px; }
    
    .grid-container { display: flex; flex-wrap: wrap; justify-content: center; gap: 10px; }
    
    .btn { 
      padding: 15px 20px; font-size: 16px; cursor: pointer; 
      border: none; border-radius: 5px; color: white; width: 100px;
      transition: background 0.3s;
    }
    .btn-on { background-color: #28a745; box-shadow: 0 4px #1e7e34; } /* Grün */
    .btn-off { background-color: #dc3545; box-shadow: 0 4px #a71d2a; } /* Rot */
    .btn:active { box-shadow: 0 2px #666; transform: translateY(2px); }

    .voltage-display { font-size: 28px; color: #007bff; font-weight: bold; margin: 10px 0; }
  </style>
</head>
<body>
  <div class="container">
    <h2>Solarhaus Zentrale</h2>
    <div class="voltage-display">Spannung: <span id="voltage">%VOLTAGE%</span> V</div>
    <div style="background: #eef; padding: 10px; border-radius: 5px; margin: 10px 0;">
        <h4>Messung (INA219)</h4>
        <div>Spannung: <b><span id="ina_v">0.00</span> V</b></div>
        <div>Strom: <b><span id="ina_ma">0.0</span> mA</b></div>
        <div>Leistung: <b><span id="ina_mw">0.0</span> mW</b></div>
    </div>

    <h3>Solarzellen (4x)</h3>
    <div class="grid-container">
      <button id="solar_0" class="btn btn-off" onclick="toggle('solar', 0)">Solar 1</button>
      <button id="solar_1" class="btn btn-off" onclick="toggle('solar', 1)">Solar 2</button>
      <button id="solar_2" class="btn btn-off" onclick="toggle('solar', 2)">Solar 3</button>
      <button id="solar_3" class="btn btn-off" onclick="toggle('solar', 3)">Solar 4</button>
    </div>

    <h3>Akkuspeicher (4x)</h3>
    <div class="grid-container">
      <button id="akku_0" class="btn btn-off" onclick="toggle('akku', 0)">Akku 1</button>
      <button id="akku_1" class="btn btn-off" onclick="toggle('akku', 1)">Akku 2</button>
      <button id="akku_2" class="btn btn-off" onclick="toggle('akku', 2)">Akku 3</button>
      <button id="akku_3" class="btn btn-off" onclick="toggle('akku', 3)">Akku 4</button>
    </div>

    <h3>Verbraucher (2x)</h3>
    <div class="grid-container">
      <button id="load_0" class="btn btn-off" onclick="toggle('load', 0)">Licht</button>
      <button id="load_1" class="btn btn-off" onclick="toggle('load', 1)">Waschm.</button>
    </div>
  </div>
  
  <script>
    // Lokaler Speicher für Zustände, um Toggle zu ermöglichen
    var states = {
      solar: [0,0,0,0],
      akku: [0,0,0,0],
      load: [0,0]
    };

    function toggle(type, idx) {
      // Zustand umkehren (0->1, 1->0)
      var newState = states[type][idx] ? 0 : 1;
      
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/set?type=" + type + "&idx=" + idx + "&state=" + newState, true);
      xhr.send();
      
      // UI sofort aktualisieren für besseres Feedback (wird später durch Server-Status korrigiert)
      updateButtonColor(type, idx, newState);
      states[type][idx] = newState;
    }

    function updateButtonColor(type, idx, state) {
      var btn = document.getElementById(type + "_" + idx);
      if(state == 1) {
        btn.className = "btn btn-on";
      } else {
        btn.className = "btn btn-off";
      }
    }

    // Regelmäßiges Abfragen der Daten
    setInterval(function() {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var json = JSON.parse(this.responseText);
          
          document.getElementById('voltage').innerHTML = json.voltage;
          if(document.getElementById('ina_v'))  document.getElementById('ina_v').innerHTML  = json.ina_v;
          if(document.getElementById('ina_ma')) document.getElementById('ina_ma').innerHTML = json.ina_ma;
          if(document.getElementById('ina_mw')) document.getElementById('ina_mw').innerHTML = json.ina_mw;
          // Arrays vom Server übernehmen und Buttons aktualisieren
          states.solar = json.solar;
          states.akku = json.akku;
          states.load = json.load;

          for(var i=0; i<4; i++) updateButtonColor('solar', i, states.solar[i]);
          for(var i=0; i<4; i++) updateButtonColor('akku', i, states.akku[i]);
          for(var i=0; i<2; i++) updateButtonColor('load', i, states.load[i]);
        }
      };
      xhr.open("GET", "/status", true);
      xhr.send();
    }, 50);
  </script>
</body>
</html>
)rawliteral";

#endif