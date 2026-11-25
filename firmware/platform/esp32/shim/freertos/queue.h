#ifndef ESP32_SHIM_QUEUE_H
#define ESP32_SHIM_QUEUE_H

#include "freertos/FreeRTOS.h"

typedef void *QueueHandle_t;

BaseType_t xQueueReceive(QueueHandle_t queue, void *item, TickType_t wait);

#endif
