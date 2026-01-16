#include "IMUSensor.h"
#include <Wire.h>
#include "GY521.h"

// Definition der Queue
QueueHandle_t sensorQueue;
extern SemaphoreHandle_t i2cMutex;
// Hardware-Pins (wie in deiner main.cpp definiert)

GY521 sensor(0x68); 

void ReadSensorTask(void *parameter) {
 
    sensor.begin(); 

    // Kalibrierung
    Serial.println("IMU Sensor: Starte Kalibrierung...");
    sensor.calibrate(100); 
    Serial.println("IMU Sensor: Kalibrierung abgeschlossen.");
    
    // Initialisiere Queue mit Null-Werten
    SensorData initialData = {0.0, 0.0, 0.0, 0.0};
    xQueueOverwrite(sensorQueue, &initialData);

    for (;;) { 
    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    sensor.read(); 
    xSemaphoreGive(i2cMutex);
}

        SensorData currentData;
        currentData.accelX = sensor.getAccelX();
        currentData.accelY = sensor.getAccelY();
        currentData.accelZ = sensor.getAccelZ();
        currentData.temperature = sensor.getTemperature();

        // Debug-Ausgabe
        Serial.printf("IMU Data: X: %.2f | Y: %.2f | Z: %.2f | T: %.1f\n", 
                      currentData.accelX, currentData.accelY, currentData.accelZ, currentData.temperature);

        // Daten in die Queue schreiben (überschreiben, damit immer die aktuellsten Daten vorliegen)
        xQueueOverwrite(sensorQueue, &currentData);
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Leseintervall: 100ms
    }
}

void startIMUSensorTask() {
    // Queue erstellen (Größe 1 für Overwrite-Modus)
    sensorQueue = xQueueCreate(1, sizeof(SensorData));
    
    // Task erstellen
    xTaskCreate(ReadSensorTask, "ReadSensor", 4096, NULL, 2, NULL);
}