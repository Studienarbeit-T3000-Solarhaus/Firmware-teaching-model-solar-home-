#include "tasks.hpp"
#include "shared_data.hpp"
#include "PinDefinitions.hpp"
#include <Arduino.h>
#include "esp_sleep.h"

void Task_DeepSleep(void* pvParameters) {
    
    unsigned long buttonPressTime = 0;
    bool isButtonPressed = false;

    while (1) {
        // D0 ist LOW, wenn der Button gedrückt wird
        if (digitalRead(WAKEUP_PIN) == LOW) {
            if (!isButtonPressed) {
                // Button wurde gerade erst gedrückt
                buttonPressTime = millis();
                isButtonPressed = true;
            } else {
                // Prüfen, ob die Haltezeit von 3 Sekunden (3000 ms) erreicht ist
                if (millis() - buttonPressTime >= 3000) {
                    #ifdef DEBUG
                    Serial.println("Button für 3 Sekunden gedrückt. Warte auf Loslassen...");
                    #endif
                    
                    // NEU: Warte, bis der Button WIRKLICH losgelassen wurde
                    while (digitalRead(WAKEUP_PIN) == LOW) {
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }
                    
                    // Ein kurzes Delay, um das Prellen beim Loslassen abzuwarten
                    vTaskDelay(pdMS_TO_TICKS(100));

                    #ifdef DEBUG
                    Serial.println("Starte Shutdown-Animation...");
                    #endif

                    // --- NEU: Herunterfahr-Animation (Rotes Faden) ---
                    // Schleife reduziert die Helligkeit der roten Farbe in 5er-Schritten von 255 auf 0
                    for (int brightness = 255; brightness >= 0; brightness -= 5) {
                        // Alle Pixel auf den aktuellen Rot-Wert setzen
                        for (int i = 0; i < Neopixels.numPixels(); i++) {
                            // Format: R, G, B. Nur Rot bekommt den Helligkeitswert.
                            Neopixels.setPixelColor(i, Neopixels.Color(brightness, 0, 0)); 
                        }
                        Neopixels.show();
                        
                        // Geschwindigkeit des Ausfadens (20ms pro Schritt = ca. 1 Sekunde für den kompletten Fade)
                        vTaskDelay(pdMS_TO_TICKS(20)); 
                    }
                    
                    // Zur Sicherheit nochmal komplett ausschalten
                    Neopixels.clear();
                    Neopixels.show();
                    // --------------------------------------------------

                    #ifdef DEBUG
                    Serial.println("Gehe in Deep Sleep...");
                    #endif

                    // Pins D6 und D7 auf LOW setzen, bevor der Deep Sleep beginnt
                    digitalWrite(ENABLE_BATTERY_PIN, LOW);
                    digitalWrite(ENABLE_3V3_PIN, LOW);
                    
                    delay(100); 

                    // Konfiguriere Wake-up für WAKEUP_PIN auf LOW
                    esp_deep_sleep_enable_gpio_wakeup(BIT(WAKEUP_PIN), ESP_GPIO_WAKEUP_GPIO_LOW);
                    
                    // Gehe in den Deep Sleep
                    esp_deep_sleep_start();
                }
            }
        } else {
            // Button wurde losgelassen, Timer zurücksetzen
            isButtonPressed = false;
        }

        // Kurzes Delay zum Entprellen und um CPU-Ressourcen freizugeben
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}