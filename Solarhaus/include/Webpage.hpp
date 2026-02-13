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
        .card { background: white; border-radius: 15px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); width: 100%; max-width: 600px; overflow: hidden; text-align: center; }
        .header { padding: 20px; background-color: #333; color: white; margin-bottom: 10px; }
        h1 { margin: 0; font-size: 24px; }
        
        /* Tabs */
        .tabs { display: flex; background: #e9ecef; border-bottom: 1px solid #ddd; }
        .tab { flex: 1; padding: 15px; cursor: pointer; font-weight: bold; color: #666; background: #f8f9fa; border: none; }
        .tab.active { background: #0077BB; color: white; }
        
        /* Sections */
        .section { padding: 20px; border-bottom: 1px solid #eee; }
        .section-title { font-weight: bold; margin-bottom: 15px; color: #333; font-size: 18px; text-align: left; }
        
        /* Controls Row */
        .control-row { display: flex; justify-content: space-between; align-items: center; background: #f8f9fa; border-radius: 10px; padding: 10px; margin-bottom: 10px; border: 1px solid #eee; }
        
        /* Buttons - Accessible Colors (Blue/Orange) */
        .btn { border: none; border-radius: 5px; padding: 10px 15px; font-weight: bold; color: white; cursor: pointer; min-width: 40px; height: 40px; font-size: 18px; display: flex; align-items: center; justify-content: center; transition: background 0.2s; }
        .btn-wide { width: auto; font-size: 14px; margin-top: 5px; }
        
        /* Blue for Positive/Active (replaces Green) */
        .btn-green { background-color: #0077BB; } 
        .btn-green:hover { background-color: #005FA3; }

        /* Orange for Negative/Inactive (replaces Red) */
        .btn-red { background-color: #D55E00; }
        .btn-red:hover { background-color: #B54D00; }

        .btn:active { opacity: 0.8; }

        .status-badge { background: #0077BB; color: white; padding: 5px 15px; border-radius: 20px; font-weight: bold; font-size: 14px; margin: 5px 0; }

        /* Consumers Grid */
        .consumer-grid { display: flex; justify-content: center; gap: 20px; }
        .consumer-btn { padding: 15px 20px; border-radius: 5px; color: white; font-weight: bold; cursor: pointer; border: none; width: 120px; }

        /* INA Data Footer */
        .footer-data { background: #f1f1f1; text-align: left; padding: 15px; font-family: monospace; font-size: 12px; color: #555; margin: 15px; border-radius: 5px; }

        /* --- Batterie Styles (Moved) --- */
        .battery-wrapper {
            display: flex;
            align-items: center;
            justify-content: space-around;
            background: #fff;
            padding: 10px;
        }

        .battery-controls {
            flex: 1;
            display: flex;
            flex-direction: column;
            gap: 10px;
            margin-right: 20px;
        }

        .control-group {
            display: flex;
            justify-content: space-between;
            align-items: center;
            background: #f8f9fa;
            padding: 10px;
            border-radius: 8px;
        }

        .battery-visual {
            flex: 0 0 auto;
        }

        .battery {
            position: relative;
            width: 70px;
            height: 140px;
            border: 4px solid #333;
            border-radius: 10px;
            padding: 3px;
            display: flex;
            flex-direction: column-reverse; 
            background: #fff;
        }

        .battery::after {
            content: '';
            position: absolute;
            top: -10px; 
            left: 50%;
            transform: translateX(-50%);
            width: 30px;
            height: 6px;
            background-color: #333;
            border-radius: 4px 4px 0 0;
        }

        .battery-level {
            width: 100%;
            height: 0%;
            background-color: #0077BB; /* Default Blue */
            border-radius: 6px;
            transition: height 0.5s ease, background-color 0.5s ease;
        }

        .percentage-text-overlay {
            position: absolute;
            top: 0; left: 0; width: 100%; height: 100%;
            display: flex; align-items: center; justify-content: center;
            font-size: 1.2rem; font-weight: bold; color: #333;
            z-index: 10;
            text-shadow: 1px 1px 0px rgba(255,255,255,0.8);
        }

    </style>
</head>
<body>
    <div class="card">
        <div class="header">
            <h1>Solar House Central</h1>
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
            <div class="battery-wrapper">
                <div class="battery-controls">
                    <div class="control-group">
                        <button class="btn btn-red" onclick="control('battery', 'dec')">-</button>
                        <span id="batteryStatus" style="font-weight:bold; color:#333;">0 / 4</span>
                        <button class="btn btn-green" onclick="control('battery', 'inc')">+</button>
                    </div>
                    <div style="display:flex; justify-content: space-between; gap: 5px;">
                        <button class="btn btn-red btn-wide" style="flex:1" onclick="control('battery', 'all_off')">OFF</button>
                        <button class="btn btn-green btn-wide" style="flex:1" onclick="control('battery', 'all_on')">MAX</button>
                    </div>
                </div>

                <div class="battery-visual">
                    <div class="battery">
                        <div class="percentage-text-overlay" id="battText">0%</div>
                        <div class="battery-level" id="battLevel"></div>
                    </div>
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
                    // Update Status Texts
                    document.getElementById('solarStatus').innerText = data.solar_count + " / 4 Active";
                    document.getElementById('batteryStatus').innerText = data.battery_count + " / 4";
                    
                    // --- Batterie Logik (1V = 0%, 6V = 100%) ---
                    const minVoltage = 1.0; 
                    const maxVoltage = 6.0; 
                    
                    let percentage = ((data.bus_voltage - minVoltage) / (maxVoltage - minVoltage)) * 100;
                    if (percentage > 100) percentage = 100;
                    if (percentage < 0) percentage = 0;

                    // Update Höhe und Text
                    const levelEl = document.getElementById('battLevel');
                    levelEl.style.height = percentage + '%';
                    document.getElementById('battText').innerText = Math.round(percentage) + '%';

                    // Farbwechsel Logik (Colorblind Safe: Blue -> Yellow -> Orange)
                    if (percentage > 50) {
                        levelEl.style.backgroundColor = '#0077BB'; // Blue (Good)
                    } else if (percentage > 20) {
                        levelEl.style.backgroundColor = '#EECC11'; // Yellow (Mid)
                    } else {
                        levelEl.style.backgroundColor = '#D55E00'; // Orange (Low)
                    }

                    // Update Buttons (Consumer)
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

        // Refresh every 100ms
        setInterval(updateUI, 100);
        window.onload = updateUI;
    </script>
</body>
</html>
)rawliteral";

#endif