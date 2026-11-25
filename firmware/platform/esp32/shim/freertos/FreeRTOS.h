#ifndef ESP32_SHIM_FREERTOS_H
#define ESP32_SHIM_FREERTOS_H

#include <stddef.h>
#include <stdint.h>

typedef void *TaskHandle_t;
typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef uint32_t TickType_t;

#define pdTRUE 1
#define pdFALSE 0
#define pdPASS 1
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#define portMAX_DELAY ((TickType_t)0xFFFFFFFFu)

#endif
