#ifndef VOLTAGEREADER_H
#define VOLTAGEREADER_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t voltageQueue; // Global zugänglich machen
void startVoltageTask();

#endif // VOLTAGEREADER_H