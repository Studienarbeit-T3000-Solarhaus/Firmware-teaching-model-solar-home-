#ifndef MOTION_SENSOR_H
#define MOTION_SENSOR_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "SensorData.h"

// Die Queue global (extern) verfügbar machen
extern QueueHandle_t sensorQueue;

void startIMUSensorTask();

#endif // MOTION_SENSOR_H