//#include <Arduino.h>
//#include <Adafruit_NeoPixel.h>
//#include "LedSegment.hpp"
//#include "Config.hpp"
//#include "PinDefinitions.hpp"
//
//// Taster-Pin definieren (wir nutzen den WAKEUP_PIN / GPIO 2 als Knopf)
//#define BUTTON_PIN WAKEUP_PIN 
//
//// Instanz des LED-Strips 
//Adafruit_NeoPixel Neopixels(NUM_NEOPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
//
//const int NUM_SEGMENTS = 16;
//LedSegment* testSegments[NUM_SEGMENTS];
//int currentSegmentIndex = 0;
//
//// Namen für die Ausgabe im Serial Monitor
//const char* segmentNames[NUM_SEGMENTS] = {
//  "1. All Solar Modules (Hauptleitung)",
//  "2. Solar Module 1", 
//  "3. Solar Module 2", 
//  "4. Solar Module 3", 
//  "5. Solar Module 4",
//  "6. All Capacitors (Hauptleitung)",
//  "7. Capacitor 1", 
//  "8. Capacitor 2", 
//  "9. Capacitor 3", 
//  "10. Capacitor 4",
//  "11. All Loads (Hauptleitung)",
//  "12. After Buck Boost",
//  "13. To Other Loads",
//  "14. Constant Load",
//  "15. Night Load",
//  "16. Heavy Load"
//};
//
//// Variablen für die Entprellung des Tasters (Debouncing)
//int buttonState = HIGH;             // Aktueller entprellter Status
//int lastButtonState = HIGH;         // Vorheriger roher Status
//unsigned long lastDebounceTime = 0;
//const unsigned long debounceDelay = 50; // 50ms Entprell-Zeit
//
//void setup() {
//  Serial.begin(115200);
//  
//  // Taster Pin initialisieren (HIGH wenn offen, LOW wenn mit GND verbunden)
//  pinMode(BUTTON_PIN, INPUT_PULLUP);
//
//  Neopixels.begin();
//  Neopixels.setBrightness(20); // Nicht zu hell für den Test
//  Neopixels.clear();
//  Neopixels.show();
//
//  // --- Alle Segmente anhand deiner Arrays aus Config.cpp initialisieren ---
//  
//  // Solar
//  testSegments[0] = new LedSegment(&Neopixels, IndicesAllSolarModules, LengthAllSolarModules, ColorCurrentflow);
//  testSegments[1] = new LedSegment(&Neopixels, solarModulesIndices[0], LengthSolarModules[0], ColorCurrentflow);
//  testSegments[2] = new LedSegment(&Neopixels, solarModulesIndices[1], LengthSolarModules[1], ColorCurrentflow);
//  testSegments[3] = new LedSegment(&Neopixels, solarModulesIndices[2], LengthSolarModules[2], ColorCurrentflow);
//  testSegments[4] = new LedSegment(&Neopixels, solarModulesIndices[3], LengthSolarModules[3], ColorCurrentflow);
//  
//  // Capacitors
//  testSegments[5] = new LedSegment(&Neopixels, IndicesAllCapacitors, LengthAllCapacitors, ColorChargeCaps);
//  testSegments[6] = new LedSegment(&Neopixels, capacitorsIndices[0], LengthCapacitors[0], ColorChargeCaps);
//  testSegments[7] = new LedSegment(&Neopixels, capacitorsIndices[1], LengthCapacitors[1], ColorChargeCaps);
//  testSegments[8] = new LedSegment(&Neopixels, capacitorsIndices[2], LengthCapacitors[2], ColorChargeCaps);
//  testSegments[9] = new LedSegment(&Neopixels, capacitorsIndices[3], LengthCapacitors[3], ColorChargeCaps);
//  
//  // Loads
//  testSegments[10] = new LedSegment(&Neopixels, IndicesAllLoads, LengthAllLoads, ColorCurrentflow);
//  testSegments[11] = new LedSegment(&Neopixels, IndicesAfterBuckBoost, LengthAfterBuckBoost, ColorCurrentflow);
//  testSegments[12] = new LedSegment(&Neopixels, IndicesToOtherLoads, LengthToOtherLoads, ColorCurrentflow);
//  testSegments[13] = new LedSegment(&Neopixels, IndicesConstantLoad, LengthConstantLoad, ColorCurrentflow);
//  testSegments[14] = new LedSegment(&Neopixels, IndicesNightLoad, LengthNightLoad, ColorCurrentflow);
//  testSegments[15] = new LedSegment(&Neopixels, IndicesHeavyLoad, LengthHeavyLoad, ColorCurrentflow);
//
//  Serial.println("\n\n--- LED Segment Test gestartet ---");
//  Serial.println("Verbinde Pin D0 (GPIO 2) kurz mit GND, um zum naechsten Segment zu wechseln.");
//  Serial.print("Aktuelles Segment: ");
//  Serial.println(segmentNames[currentSegmentIndex]);
//  
//  // Das erste Segment mit einer Animation starten (100ms Geschwindigkeit)
//  testSegments[currentSegmentIndex]->setFlow(100, false);
//}
//
//void loop() {
//  int reading = digitalRead(BUTTON_PIN);
//
//  // Wenn sich der rohe Zustand ändert, Timer zurücksetzen
//  if (reading != lastButtonState) {
//    lastDebounceTime = millis();
//  }
//
//  // Wenn der Zustand länger als 50ms stabil ist...
//  if ((millis() - lastDebounceTime) > debounceDelay) {
//    
//    // ...und wenn er sich vom aktuell gespeicherten Status unterscheidet
//    if (reading != buttonState) {
//      buttonState = reading; // Neuen Status übernehmen
//
//      // Nur reagieren, wenn der Knopf GERADE GEDRÜCKT wurde (LOW)
//      if (buttonState == LOW) {
//        // 1. Altes Segment stoppen und ausblenden
//        testSegments[currentSegmentIndex]->setFlow(0, false);
//        testSegments[currentSegmentIndex]->clear();
//        Neopixels.clear();
//
//        // 2. Nächsten Index berechnen
//        currentSegmentIndex++;
//        if (currentSegmentIndex >= NUM_SEGMENTS) {
//          currentSegmentIndex = 0; // Wieder von vorne anfangen
//        }
//
//        // 3. Neues Segment starten
//        Serial.print("Aktuelles Segment: ");
//        Serial.println(segmentNames[currentSegmentIndex]);
//        testSegments[currentSegmentIndex]->setFlow(100, false);
//      }
//    }
//  }
//  
//  // Aktuellen rohen Zustand für den nächsten Durchlauf speichern
//  lastButtonState = reading;
//
//  // 4. Update-Funktion des aktiven Segments regelmäßig aufrufen
//  testSegments[currentSegmentIndex]->update();
//  Neopixels.show();
//  
//  delay(5);
//}