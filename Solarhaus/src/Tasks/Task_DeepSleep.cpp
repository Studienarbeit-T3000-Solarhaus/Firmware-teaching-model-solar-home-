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
                // Button wird gehalten: Prüfen, ob 3 Sekunden (3000 ms) vergangen sind
                // Button wird gehalten: Prüfen, ob 3 Sekunden (3000 ms) vergangen sind
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
                    Serial.println("Gehe in Deep Sleep...");
                    #endif

                    // Pins D6 und D7 auf LOW setzen, bevor der Deep Sleep beginnt
                    digitalWrite(ENABLE_BATTERY_PIN, LOW);
                    digitalWrite(ENABLE_3V3_PIN, LOW);

                    // Konfiguriere Wake-up für D0 (GPIO 2) auf LOW
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