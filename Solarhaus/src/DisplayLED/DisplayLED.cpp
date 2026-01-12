#include "DisplayLED.h"
#include <Adafruit_NeoPixel.h>
#include "VoltageReader\VoltageReader.h" // Um Zugriff auf voltageQueue zu haben

// --- Hardware Konfiguration ---
const int NEOPIXEL_PIN = 9; 
const int NUM_PIXELS = 3; 
const float MIN_V = 0.3; // Untergrenze der Spannung für die Skala
const float MAX_V = 0.9; // Obergrenze der Spannung für die Skala

void NeoPixelTask(void *parameter) {
    // Initialisierung der NeoPixel
    Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
    pixels.begin();
    pixels.setBrightness(30); 
    pixels.clear(); 
    pixels.show();
    
    float receivedVoltage = 0.0;
    
    for (;;) { 
        // Liest den aktuellen Spannungswert aus der Queue, ohne ihn zu löschen (Peek)
        if (xQueuePeek(voltageQueue, &receivedVoltage, 0) == pdPASS) {
            
            // 1. Normalisierung der Spannung (0.0 bis 1.0)
            float normalizedValue = (receivedVoltage - MIN_V) / (MAX_V - MIN_V);
            normalizedValue = constrain(normalizedValue, 0.0, 1.0);
            
            // 2. Skalierung des Wertes für die 3 LEDs (0.0 bis 3.0)
            float scaledValue = normalizedValue * NUM_PIXELS;

            for (int i = 0; i < NUM_PIXELS; i++) {
                uint32_t color = 0;
                
                // Falls die aktuelle LED 'i' teilweise oder ganz leuchten soll
                if (scaledValue > (float)i) {
                    
                    // Berechnung des Helligkeitsfaktors für das aktuelle Segment
                    float brightnessFactor = scaledValue - (float)i;
                    brightnessFactor = constrain(brightnessFactor, 0.0, 1.0);
                    
                    // Farbverlauf bestimmen (Blau -> Grün -> Rot)
                    int blue = 0, green = 0, red = 0;

                    if (normalizedValue < 0.5) {
                        // Von Blau nach Grün
                        float mapValue = normalizedValue * 2.0; 
                        blue = (int)((1.0 - mapValue) * 255 * brightnessFactor); 
                        green = (int)(mapValue * 255 * brightnessFactor);
                    } else {
                        // Von Grün nach Rot
                        float mapValue = (normalizedValue - 0.5) * 2.0; 
                        green = (int)((1.0 - mapValue) * 255 * brightnessFactor); 
                        red = (int)(mapValue * 255 * brightnessFactor);
                    }
                    
                    color = pixels.Color(red, green, blue);
                } 
                pixels.setPixelColor(i, color); 
            }
            pixels.show();
        } 
        
        vTaskDelay(pdMS_TO_TICKS(20)); // Update-Rate ca. 50Hz
    }
}

void startLEDTask() {
    xTaskCreate(NeoPixelTask, "NeoPixelTask", 4096, NULL, 1, NULL);
}