#include "LedStrip.hpp"
#include "WebServer.h"
#include "CurrentSensor/INA3221Reader.h" 

// Definition der globalen Variablen für die Hardware-Zustände
// Diese kommen aus der Webserver.cpp
extern bool SolarMosfets[4];
extern bool BatteryMosfets[4];
extern bool LoadMosfets[2];

// Initialisierung des LED-Streifens
Adafruit_NeoPixel Mainstrip(NUMLEDS, NEOPIXELPIN, NEO_GRB + NEO_KHZ800);

// --- KONSTANTEN FÜR DIE SIMULATION ---
// Wir weisen den Geräten virtuelle Verbrauchswerte zu, 
// um zu entscheiden, ob die Batterie lädt oder entlädt.
const float SIM_WATT_SOLAR_MAX = 50.0;  // Watt pro Panel bei voller Sonne (max Voltage)
const float SIM_WATT_LAMP      = 30.0;  // Verbrauch Lampe
const float SIM_WATT_WASHING   = 150.0; // Verbrauch Waschmaschine

// Hilfsfunktion: Map für Floats
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void LedStripTask(void *parameter) {
  Mainstrip.begin();
  Mainstrip.setBrightness(10);
  Mainstrip.clear();
  Mainstrip.show();

  // Zeiger auf die Segmente
  LedSegment* solarModules[4];
  LedSegment* batteryModules[4];
  LedSegment* lampSegment;
  LedSegment* washingMachineSegment;
  LedSegment* allSolarModules;

  // Initialisiere Solar Segmente (Gelb)
  for (int i = 0; i < 4; i++) {
      solarModules[i] = new LedSegment(&Mainstrip, i * 5, 5, Mainstrip.Color(255, 200, 0)); 
  }
  allSolarModules = new LedSegment(&Mainstrip, 25, 8, Mainstrip.Color(255, 200, 0));
  // Initialisiere Batterie Segmente (Blau) - Starten bei LED 33
  for (int i = 0; i < 4; i++) {
      batteryModules[i] = new LedSegment(&Mainstrip, 33 + (i * 5), 5, Mainstrip.Color(0, 0, 255)); 
  }
  // Initialisiere Verbraucher
  lampSegment           = new LedSegment(&Mainstrip, 58, 5, Mainstrip.Color(200, 200, 200)); // Weiß (Licht)
  washingMachineSegment = new LedSegment(&Mainstrip, 63, 5, Mainstrip.Color(255, 0, 0));   // Rot (Maschine)
  
  // Variablen für die Geschwindigkeiten
  uint32_t solarSpeedMs = 0;
  uint32_t lampSpeedMs = 500;        // Langsam
  uint32_t washingSpeedMs = 100;     // Schnell (Schleudern!)

  float currentVoltageV = 0.0; // Spannung in Volt

  while(1) {
    // 1. SPANNUNG VOM INA3221 (Kanal 1) LESEN
if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    // Kanal 1 des INA3221 lesen (Index 1)
    currentVoltageV = ina3221.getBusVoltage(1); 
    xSemaphoreGive(i2cMutex);
    
    // Umrechnung für Animation: 0V bis 3V -> 1000ms bis 50ms
    float mV = currentVoltageV * 1000.0;
    solarSpeedMs = map(constrain(mV, 0, 3000), 0, 3000, 1000, 50);
} else {
    // Falls der Bus blockiert ist, behalten wir den alten Wert oder nutzen Fallback
    if (solarSpeedMs == 0) solarSpeedMs = 1000; 
}

    // 2. ENERGIEBILANZ BERECHNEN (SIMULATION)
    float totalProduction = 0.0;
    float totalConsumption = 0.0;

    // A) Produktion ermitteln
    // Annahme: Die gemessene Spannung entspricht der Sonnenintensität (0.0 - 3.0V = 0% - 100%)
    float sunIntensity = constrain(currentVoltageV / 3.0, 0.0, 1.0);
    
    for(int i=0; i<4; i++) {
        if(SolarMosfets[i]) {
            totalProduction += SIM_WATT_SOLAR_MAX * sunIntensity;
        }
    }

    // B) Verbrauch ermitteln
    if(LoadMosfets[0]) totalConsumption += SIM_WATT_LAMP;
    if(LoadMosfets[1]) totalConsumption += SIM_WATT_WASHING;

    // C) Netto-Fluss (Positiv = Überschuss, Negativ = Defizit)
    float netPower = totalProduction - totalConsumption;

    // 3. LED SEGMENTE AKTUALISIEREN

    // --- SOLAR ---
    // Solarmodule laufen immer Richtung Haus, wenn sie an sind.
    // Geschwindigkeit hängt von der globalen "Spannung/Sonne" ab.
    for (int i = 0; i < 4; i++) {
        // Wenn Mosfet an -> Fluss mit berechneter Solar-Speed, sonst 0 (aus)
        solarModules[i]->setFlow(SolarMosfets[i] ? solarSpeedMs : 0, false);
        solarModules[i]->update();
    }

    // --- BATTERIE ---
    // Hier entscheidet die Energiebilanz über Richtung und Geschwindigkeit
    bool batteryCharging = (netPower > 0); // Überschuss -> Laden -> Reverse Flow (zum Akku)
    bool batteryDischarging = (netPower < 0); // Defizit -> Entladen -> Normal Flow (vom Akku)
    
    // Geschwindigkeit der Batterie-Animation basierend auf der Menge des Überschusses/Defizits
    // Wir nehmen an: 0 bis 150 Watt Differenz mappen auf 500ms bis 50ms
    float powerDiff = abs(netPower);
    int batSpeed = 0;
    
    if (powerDiff > 1.0) { // Nur animieren bei nennenswertem Fluss
        batSpeed = map((long)constrain(powerDiff, 0, 200), 0, 200, 800, 50);
    }

    // Auf alle Akkus anwenden
    for (int i = 0; i < 4; i++) {
        if(BatteryMosfets[i]) {
            if (batteryCharging) {
                // Überschuss: Fluss ZUM Akku (reverse = false)
                batteryModules[i]->setFlow(batSpeed, false); 
                // Optional: Farbe ändern beim Laden? (z.B. Grün statt Blau)
                batteryModules[i]->setColor(Mainstrip.Color(0, 255, 0)); // Grün beim Laden
            } 
            else if (batteryDischarging) {
                // Mangel: Fluss VOM Akku (reverse = true)
                batteryModules[i]->setFlow(batSpeed, true);
                batteryModules[i]->setColor(Mainstrip.Color(0, 0, 255)); // Blau beim Entladen
            } 
            else {
                // Ausgeglichen (netPower ca 0) -> Keine Animation
                batteryModules[i]->setFlow(0, false);
            }
        } else {
            // Akku ausgeschaltet
            batteryModules[i]->setFlow(0, false);
        }
        batteryModules[i]->update();
    }

    // --- VERBRAUCHER ---
    // Lampe
    lampSegment->setFlow(LoadMosfets[0] ? lampSpeedMs : 0, false);
    lampSegment->update();
    
    // Waschmaschine
    washingMachineSegment->setFlow(LoadMosfets[1] ? washingSpeedMs : 0, false);
    washingMachineSegment->update();

    // Alles anzeigen
    Mainstrip.show(); 
    vTaskDelay(pdMS_TO_TICKS(20)); // ca. 50Hz Update-Rate
  }
}

void startLedStripTask() {
  xTaskCreate(LedStripTask, "LedStripTask", 4096, NULL, 1, NULL);
}