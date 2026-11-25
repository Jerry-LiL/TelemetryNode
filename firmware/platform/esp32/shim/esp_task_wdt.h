#ifndef ESP32_SHIM_TASK_WDT_H
#define ESP32_SHIM_TASK_WDT_H

#include <stdint.h>

#ifndef ESP_OK
typedef int esp_err_t;
#define ESP_OK 0
#endif

typedef struct {
    uint32_t timeout_ms;
    uint32_t idle_core_mask;
    int trigger_panic;
} esp_task_wdt_config_t;

esp_err_t esp_task_wdt_reconfigure(const esp_task_wdt_config_t *cfg);
esp_err_t esp_task_wdt_add(void *task);
esp_err_t esp_task_wdt_reset(void);

#endif
