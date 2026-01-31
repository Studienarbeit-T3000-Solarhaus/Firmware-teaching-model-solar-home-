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
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; text-align: center; margin: 0; padding: 20px; background-color: #f0f2f5; }
    .container { max-width: 600px; margin: 0 auto; background: white; padding: 25px; border-radius: 15px; box-shadow: 0 4px 15px rgba(0,0,0,0.1); }
    h3 { margin-top: 20px; color: #333; border-bottom: 2px solid #eee; padding-bottom: 10px; }
    
    .control-panel { 
      display: flex; align-items: center; justify-content: space-between; 
      background: #fafafa; padding: 15px; border-radius: 12px; margin-bottom: 20px; border: 1px solid #e0e0e0;
    }

    .status-icon { font-size: 40px; margin-bottom: 5px; display: block; }
    .status-container { flex: 1; text-align: center; }
    .counter-badge { 
      background: #007bff; color: white; padding: 2px 10px; border-radius: 10px; 
      font-weight: bold; font-size: 14px; display: inline-block;
    }

    .btn-group { display: flex; flex-direction: column; gap: 8px; }
    .btn { 
      padding: 10px 15px; font-size: 14px; cursor: pointer; border: none; 
      border-radius: 6px; color: white; transition: all 0.2s; font-weight: bold;
    }
    
    .btn-plus { background-color: #28a745; }
    .btn-minus { background-color: #dc3545; }
    
    .btn-all-on { background-color: #218838; font-size: 11px; }
    .btn-all-on:hover { background-color: #1e7e34; }
    
    .btn-all-off { background-color: #c82333; font-size: 11px; }
    .btn-all-off:hover { background-color: #bd2130; }

    .btn:active { transform: translateY(2px); opacity: 0.8; }
    .btn:disabled { background-color: #ccc; cursor: not-allowed; }

    .voltage-display { font-size: 32px; color: #007bff; font-weight: bold; margin: 15px 0; }

    .load-container { display: flex; gap: 10px; justify-content: center; }
    .btn-load { width: 100px; height: 50px; background-color: #dc3545; }
    .btn-load.on { background-color: #28a745; }
  </style>
</head>
<body>
  <div class="container">
    <h2>Solarhaus Zentrale</h2>
    
    <div class="voltage-display"><span id="voltage">%VOLTAGE%</span> V</div>

    <h3>Solarzellen</h3>
    <div class="control-panel">
      <div class="btn-group">
        <button class="btn btn-plus" onclick="adjustCount('solar', 1)">+</button>
        <button class="btn btn-minus" onclick="adjustCount('solar', -1)">-</button>
      </div>
      
      <div class="status-container">
        <span class="status-icon"></span>
        <div id="solar-counter" class="counter-badge">0 / 4 Aktiv</div>
      </div>

      <div class="btn-group">
        <button class="btn btn-all-on" onclick="setAll('solar', 1)">Alle AN</button>
        <button class="btn btn-all-off" onclick="setAll('solar', 0)">Alle AUS</button>
      </div>
    </div>

    <h3>Akkuspeicher</h3>
    <div class="control-panel">
      <div class="btn-group">
        <button class="btn btn-plus" onclick="adjustCount('akku', 1)">+</button>
        <button class="btn btn-minus" onclick="adjustCount('akku', -1)">-</button>
      </div>

      <div class="status-container">
        <span class="status-icon"></span>
        <div id="akku-counter" class="counter-badge">0 / 4 Aktiv</div>
      </div>

      <div class="btn-group">
        <button class="btn btn-all-on" onclick="setAll('akku', 1)">Alle AN</button>
        <button class="btn btn-all-off" onclick="setAll('akku', 0)">Alle AUS</button>
      </div>
    </div>

    <h3>Verbraucher</h3>
    <div class="load-container">
      <button id="load_0" class="btn btn-load" onclick="toggleLoad(0)">Licht</button>
      <button id="load_1" class="btn btn-load" onclick="toggleLoad(1)">Waschm.</button>
    </div>

    <div style="margin-top:20px; font-size: 12px; color: #666; background: #eee; padding: 10px; border-radius: 5px;">
        INA3221: <span id="ina_v">0.0</span>V | <span id="ina_ma">0</span>mA | <span id="ina_mw">0</span>mW
    </div>
  </div>
  
  <script>
    var states = { solar: [0,0,0,0], akku: [0,0,0,0], load: [0,0] };

    function adjustCount(type, change) {
      let currentActive = states[type].filter(s => s === 1).length;
      let targetActive = Math.max(0, Math.min(4, currentActive + change));
      
      for (let i = 0; i < 4; i++) {
        let newState = (i < targetActive) ? 1 : 0;
        if (states[type][i] !== newState) {
          states[type][i] = newState; 
          sendRequest(type, i, newState);
        }
      }
      updateUI();
    }

    function setAll(type, state) {
      for (let i = 0; i < 4; i++) {
        states[type][i] = state; 
        sendRequest(type, i, state);
      }
      updateUI(); 
    }

    function toggleLoad(idx) {
      let newState = states.load[idx] ? 0 : 1;
      sendRequest('load', idx, newState);
    }

    function sendRequest(type, idx, state) {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/set?type=" + type + "&idx=" + idx + "&state=" + state, true);
      xhr.send();
      states[type][idx] = state;
      updateUI();
    }

    function updateUI() {
      let solarActive = states.solar.filter(s => s === 1).length;
      let akkuActive = states.akku.filter(s => s === 1).length;
      
      document.getElementById('solar-counter').innerHTML = solarActive + " / 4 Aktiv";
      document.getElementById('akku-counter').innerHTML = akkuActive + " / 4 Aktiv";
      
      for(let i=0; i<2; i++) {
        let btn = document.getElementById('load_' + i);
        btn.className = states.load[i] ? "btn btn-load on" : "btn btn-load";
      }
    }

    setInterval(function() {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var json = JSON.parse(this.responseText);
          
          document.getElementById('voltage').innerHTML = json.voltage;
          document.getElementById('ina_v').innerHTML = json.ina_v;
          document.getElementById('ina_ma').innerHTML = json.ina_ma;
          document.getElementById('ina_mw').innerHTML = json.ina_mw;
          
          states.solar = json.solar;
          states.akku = json.akku;
          states.load = json.load;
          updateUI();
        }
      };
      xhr.open("GET", "/status", true);
      xhr.send();
    }, 100);
  </script>
</body>
</html>
)rawliteral";

#endif