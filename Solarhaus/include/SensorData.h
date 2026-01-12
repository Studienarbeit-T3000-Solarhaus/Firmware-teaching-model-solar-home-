#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <Arduino.h>

// Struktur für die Sensordaten: Beschleunigung (X, Y, Z) und Temperatur (T)
struct SensorData {
    float accelX;
    float accelY;
    float accelZ;
    float temperature;
};

#endif // SENSOR_DATA_H