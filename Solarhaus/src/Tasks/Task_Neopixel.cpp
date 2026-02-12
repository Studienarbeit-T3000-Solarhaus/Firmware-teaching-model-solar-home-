#include "tasks.hpp"
#include "shared_data.hpp"
#include "Config.hpp"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

void Task_Neopixel(void* pvParameters) {
    // Lokale Variablen für das Lauflicht
    int pixelIndex = 0;
    const int numActivePixels = 3; // Wir nutzen nur die ersten 3 Pixel

    while(1) {
        float current_mA = 0.0f;
        
        // 1. Aktuellen Stromwert holen (Thread-safe)
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            current_mA = sysState.current_mA[0]; // Channel 0 (Solar)
            xSemaphoreGive(dataMutex);
        }

        // 2. Lauflicht Logik
        int delayTime = 0;
        
        // Schwellwert: Unter 1mA ist das Lauflicht aus
        if (current_mA < 1.0) {
            pixels.clear();
            pixels.show();
            delayTime = 500; // Langsam prüfen, wenn inaktiv
            pixelIndex = 0;  // Reset
        } else {
            // Begrenzung für die Berechnung auf max 30mA
            float calcCurrent = (current_mA > 30.0) ? 30.0 : current_mA;
            
            // Mapping: 
            // Wenig Strom (1mA) -> Langsam (200ms Verzögerung)
            // Viel Strom (30mA) -> Schnell (30ms Verzögerung)
            delayTime = map((long)calcCurrent, 1, 30, 200, 30);
            
            pixels.clear();
            // Setze Farbe (z.B. Blau/Cyan)
            pixels.setPixelColor(pixelIndex, pixels.Color(0, 100, 150)); 
            pixels.show();
            
            // Zähler erhöhen
            pixelIndex++;
            if (pixelIndex >= numActivePixels) {
                pixelIndex = 0;
            }
        }
        
        // Wartezeit bestimmt die Geschwindigkeit des Lauflichts
        vTaskDelay(pdMS_TO_TICKS(delayTime));
    }
}