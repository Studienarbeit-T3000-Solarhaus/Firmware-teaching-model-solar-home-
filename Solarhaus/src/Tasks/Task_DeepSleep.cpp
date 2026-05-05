#include "tasks.hpp"
#include "shared_data.hpp"
#include "PinDefinitions.hpp"
#include <Arduino.h>
#include "esp_sleep.h"
#include "DebugConfig.hpp"

// Monitors power button: 3s long-press triggers shutdown animation and enters deep sleep
void Task_DeepSleep(void* pvParameters) {
    
    unsigned long buttonPressTime = 0;
    bool isButtonPressed = false;

    while (1) {
        if (digitalRead(WAKEUP_PIN) == LOW) {
            if (!isButtonPressed) {
                buttonPressTime = millis();
                isButtonPressed = true;
            } else {
                // 3-second hold detected — initiate shutdown sequence
                if (millis() - buttonPressTime >= 3000) {
                    #ifdef DEBUG
                    Serial.println("Button held 3s. Waiting for release...");
                    #endif
                    
                    // Wait for button release (with debounce)
                    while (digitalRead(WAKEUP_PIN) == LOW) {
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }
                    vTaskDelay(pdMS_TO_TICKS(100));

                    #ifdef DEBUG
                    Serial.println("Starting shutdown animation...");
                    #endif
                    if (xSemaphoreTake(NeoPixelMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    // Red fade-out animation (~1s) as visual shutdown indicator
                    for (int brightness = 255; brightness >= 0; brightness -= 5) {
                        for (int i = 0; i < Neopixels.numPixels(); i++) {
                            Neopixels.setPixelColor(i, Neopixels.Color(brightness, 0, 0)); 
                        }
                        Neopixels.show();
                        vTaskDelay(pdMS_TO_TICKS(20)); 
                    }
                    
                    Neopixels.clear();
                    Neopixels.show();
                    
                    #ifdef DEBUG
                    Serial.println("Entering deep sleep...");
                    #endif

                    // Disable power rails before sleeping
                    digitalWrite(ENABLE_BATTERY_PIN, LOW);
                    digitalWrite(ENABLE_3V3_PIN, LOW);
                    
                    delay(100); 
                    xSemaphoreGive(NeoPixelMutex);
                    }

                    // Configure GPIO wake-up source and enter deep sleep
                    esp_deep_sleep_enable_gpio_wakeup(BIT(WAKEUP_PIN), ESP_GPIO_WAKEUP_GPIO_LOW);
                    esp_deep_sleep_start();
                }
            }
        } else {
            isButtonPressed = false;
        }

        vTaskDelay(pdMS_TO_TICKS(PERIOD_DEEPSLEEP_TASK));
    }
}