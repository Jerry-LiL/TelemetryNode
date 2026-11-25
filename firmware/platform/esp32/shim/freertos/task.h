#ifndef ESP32_SHIM_TASK_H
#define ESP32_SHIM_TASK_H

#include "freertos/FreeRTOS.h"

BaseType_t xTaskCreate(void (*fn)(void *), const char *name, uint32_t stack,
                       void *arg, UBaseType_t priority, TaskHandle_t *created);

#endif
