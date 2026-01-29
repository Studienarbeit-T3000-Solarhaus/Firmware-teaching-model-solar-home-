#include "INA3221Reader.h"
// Initialisierung mit Standard-I2C-Adresse 0x40


void INA3221Task(void *parameter) {
    ina3221.begin();
    
    for (;;) {
        // Sicherer Zugriff auf den I2C-Bus mittels Mutex
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            Serial.println("--- INA3221 Messwerte ---");
            
        for (int i = 0; i <= 2; i++) {
        float v = ina3221.getBusVoltage(i);
        float ma = ina3221.getCurrentAmps(i) * 1000;
                
        // %4.4f zeigt 4 Nachkommastellen für Volt an
        // %4.2f zeigt 2 Nachkommastellen für mA an
        Serial.printf("Kanal %d: %4.4f V | %4.4f mA\n", i, v, ma);
        }
    Serial.println("-------------------------");
            xSemaphoreGive(i2cMutex);
        }

        // Alle 2000ms (2 Sekunden) eine Messung ausgeben
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void startINA3221Task() {
    xTaskCreate(INA3221Task, "INA3221Task", 4096, NULL, 1, NULL);
}