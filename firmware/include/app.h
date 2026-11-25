#ifndef TELEMETRY_APP_H
#define TELEMETRY_APP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void app_set_boot(uint16_t boot_count, uint8_t node_id, const char *reason);
void app_init(void);
void app_create_tasks(void);

#ifdef __cplusplus
}
#endif

#endif
