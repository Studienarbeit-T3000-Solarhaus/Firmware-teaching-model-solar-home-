#ifndef WEBPAGE_HPP
#define WEBPAGE_HPP

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Solar House Power Monitor</title>
    <style>
        body { font-family: 'Segoe UI', Arial, sans-serif; background-color: #f0f2f5; margin: 0; padding: 20px; display: flex; justify-content: center; }
        .card { background: white; border-radius: 15px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); width: 100%; max-width: 600px; overflow: hidden; text-align: center; display: flex; flex-direction: column; }
        
        /* Header & Tabs */
        .header { background-color: #333; color: white; padding: 20px 20px 0 20px; }
        h1 { margin: 0 0 15px 0; font-size: 24px; }

        .tab-bar { display: flex; justify-content: space-around; background-color: #222; }
        .tab-btn { background: none; border: none; outline: none; cursor: pointer; padding: 14px 10px; transition: 0.3s; color: #bbb; font-size: 15px; font-weight: bold; flex: 1; border-bottom: 3px solid transparent; }
        .tab-btn:hover { color: white; background-color: #444; }
        .tab-btn.active { color: white; border-bottom: 3px solid #0077BB; background-color: #333; }

        /* Content Sections */
        .tab-content { display: none; padding: 20px; text-align: left; animation: fadeEffect 0.5s; }
        .tab-content.active { display: block; }
        
        @keyframes fadeEffect { from {opacity: 0;} to {opacity: 1;} }

        .section { padding: 15px 0; border-bottom: 1px solid #eee; }
        .section:last-child { border-bottom: none; }
        .section-title { font-weight: bold; margin-bottom: 10px; color: #333; font-size: 18px; display: flex; justify-content: space-between; align-items: center; }
        
        /* Data Badge Style */
        .live-data { font-family: monospace; font-size: 12px; background: #eee; padding: 5px 8px; border-radius: 5px; color: #333; white-space: nowrap; }
        
        /* Controls Row */
        .control-row { display: flex; justify-content: space-between; align-items: center; background: #f8f9fa; border-radius: 10px; padding: 10px; border: 1px solid #eee; margin-top: 10px; }
        
        .btn { border: none; border-radius: 5px; padding: 10px 15px; font-weight: bold; color: white; cursor: pointer; min-width: 40px; height: 40px; font-size: 18px; display: flex; align-items: center; justify-content: center; transition: background 0.2s; }
        .btn-wide { width: auto; font-size: 14px; margin-top: 5px; }
        .btn-green { background-color: #0077BB; } .btn-green:hover { background-color: #005FA3; }
        .btn-red { background-color: #D55E00; } .btn-red:hover { background-color: #B54D00; }
        .btn-gray { background-color: #666; } .btn-gray:hover { background-color: #555; }
        
        .status-badge { background: #0077BB; color: white; padding: 5px 15px; border-radius: 20px; font-weight: bold; font-size: 14px; margin: 5px 0; }

        /* Consumers Grid */
        .consumer-container { display: flex; justify-content: space-around; gap: 10px; text-align: center; margin-top: 10px; }
        .consumer-item { display: flex; flex-direction: column; align-items: center; width: 30%; }
        .consumer-btn { padding: 15px 5px; border-radius: 5px; color: white; font-weight: bold; cursor: pointer; border: none; width: 100%; margin-bottom: 5px; font-size: 14px; }
        .theo-power { font-size: 11px; color: #777; }

        /* Battery Styles */
        .battery-wrapper { display: flex; align-items: center; justify-content: space-around; background: #fff; padding: 10px 0; }
        .battery-controls { flex: 1; display: flex; flex-direction: column; gap: 10px; margin-right: 20px; }
        .control-group { display: flex; justify-content: space-between; align-items: center; background: #f8f9fa; padding: 10px; border-radius: 8px; }
        .battery { position: relative; width: 70px; height: 140px; border: 4px solid #333; border-radius: 10px; padding: 3px; display: flex; flex-direction: column-reverse; background: #fff; }
        .battery::after { content: ''; position: absolute; top: -10px; left: 50%; transform: translateX(-50%); width: 30px; height: 6px; background-color: #333; border-radius: 4px 4px 0 0; }
        .battery-level { width: 100%; height: 0%; background-color: #0077BB; border-radius: 6px; transition: height 0.5s ease, background-color 0.5s ease; }
        .percentage-text-overlay { position: absolute; top: 0; left: 0; width: 100%; height: 100%; display: flex; align-items: center; justify-content: center; font-size: 1.2rem; font-weight: bold; color: #333; z-index: 10; text-shadow: 1px 1px 0px rgba(255,255,255,0.8); }

        /* Info Tab Table */
        .info-table { width: 100%; border-collapse: collapse; font-size: 14px; }
        .info-table td { padding: 8px; border-bottom: 1px solid #eee; }
        .info-table td:first-child { font-weight: bold; color: #555; }
        .info-table td:last-child { text-align: right; font-family: monospace; }

    </style>
</head>
<body>
    <div class="card">
        <div class="header">
            <h1>Solar House Power Monitor</h1>
        </div>
        
        <div class="tab-bar">
            <button class="tab-btn active" onclick="openTab(event, 'ManualMode')">Manual Mode</button>
            <button class="tab-btn" onclick="openTab(event, 'DayNightCycle')">Day/Night-Cycle</button>
            <button class="tab-btn" onclick="openTab(event, 'SystemInfo')">System Info</button>
        </div>

        <div id="ManualMode" class="tab-content active">
            
            <div class="section">
                <div class="section-title">
                    Solar Panels
                    <span class="live-data" id="solarDataDisplay">0.0V | 0mA | 0mW</span>
                </div>
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
                <div class="section-title">
                    Battery Storage
                    <span class="live-data" id="battDataDisplay">0.0V | 0mA | 0mW</span>
                </div>
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
                <div class="section-title">
                    Consumers
                    <span class="live-data" id="loadDataDisplay">0.0V | 0mA | 0mW</span>
                </div>
                
                <div class="consumer-container">
                    <div class="consumer-item">
                        <button id="btnConst" class="consumer-btn btn-red" onclick="toggleLoad('const')">Constant Load</button>
                        <span class="theo-power">Est. Power: ~20mW</span>
                    </div>

                    <div class="consumer-item">
                        <button id="btnNight" class="consumer-btn btn-red" onclick="toggleLoad('night')">Night Light</button>
                        <span class="theo-power">Est. Power: ~100mW</span>
                    </div>

                    <div class="consumer-item">
                        <button id="btnHeavy" class="consumer-btn btn-red" onclick="toggleLoad('heavy')">Heavy Machine</button>
                        <span class="theo-power">Est. Power: ~500mW</span>
                    </div>
                </div>
            </div>
        </div>

        <div id="DayNightCycle" class="tab-content">
            <div class="section">
                <div class="section-title">Automatic Simulation Control</div>
                <p style="color:#666; font-size:14px; margin-bottom:20px;">
                    Start the automatic simulation to cycle through day and night phases. 
                    The system will automatically adjust solar input and loads.
                </p>
                
                <div class="control-row" style="justify-content: center; gap: 15px; padding: 20px;">
                    <button class="btn btn-green" style="width:120px;" onclick="alert('Simulation Start - Logic needed')">START</button>
                    <button class="btn btn-red" style="width:120px;" onclick="alert('Simulation Stop - Logic needed')">STOP</button>
                </div>

                <div class="section-title" style="margin-top:20px;">Simulation Speed</div>
                <div style="background:#f8f9fa; padding:15px; border-radius:10px; border:1px solid #eee;">
                    <input type="range" min="1" max="10" value="5" style="width:100%; cursor:pointer;">
                    <div style="display:flex; justify-content:space-between; font-size:12px; color:#555; margin-top:5px;">
                        <span>Slow (Realtime)</span>
                        <span>Fast (Demo)</span>
                    </div>
                </div>
            </div>
        </div>

        <div id="SystemInfo" class="tab-content">
            <div class="section-title">System Metrics</div>
            <table class="info-table">
                <tr><td>Bus Voltage (Solar)</td><td id="info_v1">0.00 V</td></tr>
                <tr><td>Bus Voltage (Battery)</td><td id="info_v2">0.00 V</td></tr>
                <tr><td>Bus Voltage (Load)</td><td id="info_v3">0.00 V</td></tr>
                <tr><td>Total Power Draw</td><td id="info_ptotal">0 mW</td></tr>
                <tr><td>Status</td><td style="color:green; font-weight:bold;">ONLINE</td></tr>
            </table>
            <br>
            <div style="font-size:12px; color:#999; text-align:center;">
                Solar House Firmware v1.1<br>
                Powered by ESP32-C3 & FreeRTOS
            </div>
        </div>
    </div>

    <script>
        function openTab(evt, tabName) {
            var i, tabcontent, tablinks;
            tabcontent = document.getElementsByClassName("tab-content");
            for (i = 0; i < tabcontent.length; i++) {
                tabcontent[i].style.display = "none"; 
                tabcontent[i].classList.remove("active");
            }
            tablinks = document.getElementsByClassName("tab-btn");
            for (i = 0; i < tablinks.length; i++) {
                tablinks[i].className = tablinks[i].className.replace(" active", "");
            }
            document.getElementById(tabName).style.display = "block";
            document.getElementById(tabName).classList.add("active");
            evt.currentTarget.className += " active";
        }

        function updateUI() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    // --- Solar Section (CH1 in JSON / Channel 0) ---
                    document.getElementById('solarStatus').innerText = data.solar_count + " / 4 Active";
                    document.getElementById('solarDataDisplay').innerText = 
                        data.ch1_v.toFixed(2) + "V | " + data.ch1_ma.toFixed(0) + "mA | " + data.ch1_mw.toFixed(0) + "mW";

                    // --- Battery Section (CH2 in JSON / Channel 1) ---
                    document.getElementById('batteryStatus').innerText = data.battery_count + " / 4";
                    document.getElementById('battDataDisplay').innerText = 
                        data.ch2_v.toFixed(2) + "V | " + data.ch2_ma.toFixed(0) + "mA | " + data.ch2_mw.toFixed(0) + "mW";

                    // Battery Visual
                    const minVoltage = 1.0; 
                    const maxVoltage = 6.0; 
                    let percentage = ((data.bus_voltage - minVoltage) / (maxVoltage - minVoltage)) * 100;
                    if (percentage > 100) percentage = 100;
                    if (percentage < 0) percentage = 0;

                    const levelEl = document.getElementById('battLevel');
                    levelEl.style.height = percentage + '%';
                    document.getElementById('battText').innerText = Math.round(percentage) + '%';

                    if (percentage > 50) levelEl.style.backgroundColor = '#0077BB'; 
                    else if (percentage > 20) levelEl.style.backgroundColor = '#EECC11'; 
                    else levelEl.style.backgroundColor = '#D55E00'; 

                    // --- Consumer Section (CH3 in JSON / Channel 2) ---
                    document.getElementById('loadDataDisplay').innerText = 
                        data.ch3_v.toFixed(2) + "V | " + data.ch3_ma.toFixed(0) + "mA | " + data.ch3_mw.toFixed(0) + "mW";

                    // Update Button Colors
                    document.getElementById('btnConst').className = data.const_on ? 'consumer-btn btn-green' : 'consumer-btn btn-red';
                    document.getElementById('btnNight').className = data.night_on ? 'consumer-btn btn-green' : 'consumer-btn btn-red';
                    document.getElementById('btnHeavy').className = data.heavy_on ? 'consumer-btn btn-green' : 'consumer-btn btn-red';

                    // --- Info Tab Updates ---
                    document.getElementById('info_v1').innerText = data.ch1_v.toFixed(3) + " V";
                    document.getElementById('info_v2').innerText = data.ch2_v.toFixed(3) + " V";
                    document.getElementById('info_v3').innerText = data.ch3_v.toFixed(3) + " V";
                    document.getElementById('info_ptotal').innerText = (data.ch1_mw + data.ch2_mw + data.ch3_mw).toFixed(0) + " mW";
                });
        }

        function control(type, action) {
            fetch(`/api/control?type=${type}&action=${action}`).then(() => updateUI());
        }

        function toggleLoad(load) {
            fetch(`/api/toggle?load=${load}`).then(() => updateUI());
        }

        setInterval(updateUI, 50);
        window.onload = updateUI;
    </script>
</body>
</html>
)rawliteral";

#endif