#ifndef WEBPAGE_HPP
#define WEBPAGE_HPP

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Solar House Control</title>
    <style>
        body { font-family: Arial, sans-serif; background-color: #f0f2f5; margin: 0; padding: 20px; display: flex; justify-content: center; }
        .card { background: white; border-radius: 15px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); width: 100%; max-width: 500px; overflow: hidden; text-align: center; }
        .header { padding: 20px; }
        h1 { margin: 0; font-size: 24px; color: #000; }
        .voltage-display { font-size: 48px; font-weight: bold; color: #007bff; margin: 10px 0; }
        
        /* Tabs */
        .tabs { display: flex; background: #e9ecef; border-bottom: 1px solid #ddd; }
        .tab { flex: 1; padding: 15px; cursor: pointer; font-weight: bold; color: #666; background: #f8f9fa; border: none; }
        .tab.active { background: #007bff; color: white; }
        
        /* Sections */
        .section { padding: 20px; border-bottom: 1px solid #eee; }
        .section-title { font-weight: bold; margin-bottom: 15px; color: #333; font-size: 18px; }
        
        /* Controls Row */
        .control-row { display: flex; justify-content: space-between; align-items: center; background: #f8f9fa; border-radius: 10px; padding: 10px; margin-bottom: 10px; border: 1px solid #eee; }
        
        /* Buttons */
        .btn { border: none; border-radius: 5px; padding: 10px 15px; font-weight: bold; color: white; cursor: pointer; width: 40px; height: 40px; font-size: 18px; display: flex; align-items: center; justify-content: center; }
        .btn-wide { width: auto; font-size: 14px; margin-left: 5px; }
        .btn-green { background-color: #28a745; }
        .btn-red { background-color: #dc3545; }
        .btn-gray { background-color: #6c757d; }
        .btn:active { opacity: 0.8; }

        .status-badge { background: #007bff; color: white; padding: 5px 15px; border-radius: 20px; font-weight: bold; font-size: 14px; }

        /* Consumers Grid */
        .consumer-grid { display: flex; justify-content: center; gap: 20px; }
        .consumer-btn { padding: 15px 20px; border-radius: 5px; color: white; font-weight: bold; cursor: pointer; border: none; width: 100px; }

        /* INA Data Footer */
        .footer-data { background: #f1f1f1; text-align: left; padding: 15px; font-family: monospace; font-size: 12px; color: #555; margin: 15px; border-radius: 5px; }
    </style>
</head>
<body>
    <div class="card">
        <div class="header">
            <h1>Solar House Central</h1>
            <div class="voltage-display" id="mainVoltage">0.000 V</div>
        </div>

        <div class="tabs">
            <button class="tab active">Control</button>
            </div>

        <div class="section">
            <div class="section-title">Solar Cells</div>
            <div class="control-row">
                <div style="display:flex; flex-direction:column; gap:5px;">
                    <button class="btn btn-green" onclick="control('solar', 'inc')">+</button>
                    <button class="btn btn-red" onclick="control('solar', 'dec')">-</button>
                </div>
                <div class="status-badge" id="solarStatus">0 / 4 Active</div>
                <div style="display:flex; flex-direction:column; gap:5px;">
                    <button class="btn btn-green btn-wide" onclick="control('solar', 'all_on')">All ON</button>
                    <button class="btn btn-red btn-wide" onclick="control('solar', 'all_off')">All OFF</button>
                </div>
            </div>
        </div>

        <div class="section">
            <div class="section-title">Battery Storage</div>
            <div class="control-row">
                <div style="display:flex; flex-direction:column; gap:5px;">
                    <button class="btn btn-green" onclick="control('battery', 'inc')">+</button>
                    <button class="btn btn-red" onclick="control('battery', 'dec')">-</button>
                </div>
                <div class="status-badge" id="batteryStatus">0 / 4 Active</div>
                <div style="display:flex; flex-direction:column; gap:5px;">
                    <button class="btn btn-green btn-wide" onclick="control('battery', 'all_on')">All ON</button>
                    <button class="btn btn-red btn-wide" onclick="control('battery', 'all_off')">All OFF</button>
                </div>
            </div>
        </div>

        <div class="section">
            <div class="section-title">Consumers</div>
            <div class="consumer-grid">
                <button id="btnLight" class="consumer-btn btn-red" onclick="toggleLoad('light')">Light</button>
                <button id="btnMachine" class="consumer-btn btn-red" onclick="toggleLoad('machine')">Washing M.</button>
            </div>
        </div>

        <div class="footer-data" id="inaData">
            Loading sensor data...
        </div>
    </div>

    <script>
        function updateUI() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('mainVoltage').innerText = data.bus_voltage.toFixed(3) + " V";
                    document.getElementById('solarStatus').innerText = data.solar_count + " / 4 Active";
                    document.getElementById('batteryStatus').innerText = data.battery_count + " / 4 Active";
                    
                    // Update Load Buttons colors
                    const btnLight = document.getElementById('btnLight');
                    btnLight.className = data.light_on ? 'consumer-btn btn-green' : 'consumer-btn btn-red';
                    
                    const btnMachine = document.getElementById('btnMachine');
                    btnMachine.className = data.machine_on ? 'consumer-btn btn-green' : 'consumer-btn btn-red';

                    // Update Footer
                    let html = "<strong>INA3221 Channels:</strong><br>";
                    html += `CH1 (Solar): ${data.ch1_v.toFixed(2)}V | ${data.ch1_ma.toFixed(0)}mA | ${data.ch1_mw.toFixed(0)}mW<br>`;
                    html += `CH2 (Bat):   ${data.ch2_v.toFixed(2)}V | ${data.ch2_ma.toFixed(0)}mA | ${data.ch2_mw.toFixed(0)}mW<br>`;
                    html += `CH3 (Load):  ${data.ch3_v.toFixed(2)}V | ${data.ch3_ma.toFixed(0)}mA | ${data.ch3_mw.toFixed(0)}mW<br>`;
                    document.getElementById('inaData').innerHTML = html;
                });
        }

        function control(type, action) {
            fetch(`/api/control?type=${type}&action=${action}`).then(() => updateUI());
        }

        function toggleLoad(load) {
            fetch(`/api/toggle?load=${load}`).then(() => updateUI());
        }

        // Refresh every 2 seconds
        setInterval(updateUI, 100);
        window.onload = updateUI;
    </script>
</body>
</html>
)rawliteral";

#endif