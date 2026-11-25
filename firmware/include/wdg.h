#ifndef TELEMETRY_WDG_H
#define TELEMETRY_WDG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TN_TASK_ENV = 0,
    TN_TASK_IMU = 1,
    TN_TASK_TELEM = 2,
    TN_TASK_SUPERVISOR = 3,
    TN_TASK_COUNT = 4
} tn_task_id;

enum {
    WDG_CAUSE_STALL = 1
};

/*
 * Bits in active_mask select the tasks to check. A task is stale when
 * (now_ms - last_kick_ms) > timeout_ms. Unsigned subtraction wraps cleanly.
 */
uint32_t wdg_stale_mask(const uint32_t *last_kick_ms, uint32_t now_ms,
                        uint32_t timeout_ms, uint32_t active_mask);

/* Tasks the supervisor restarts. The supervisor itself is covered by the IWDG. */
#define WDG_SUPERVISED_MASK \
    ((1u << TN_TASK_ENV) | (1u << TN_TASK_IMU) | (1u << TN_TASK_TELEM))

#ifdef __cplusplus
}
#endif

#endif
