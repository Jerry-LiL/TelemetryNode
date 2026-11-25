#include "wdg.h"

#include <stddef.h>

uint32_t wdg_stale_mask(const uint32_t *last_kick_ms, uint32_t now_ms,
                        uint32_t timeout_ms, uint32_t active_mask)
{
    uint32_t stale = 0u;
    int id;

    if (last_kick_ms == NULL) {
        return active_mask;
    }

    for (id = 0; id < TN_TASK_COUNT; id++) {
        uint32_t bit = 1u << id;
        uint32_t age;
        if ((active_mask & bit) == 0u) {
            continue;
        }
        age = now_ms - last_kick_ms[id];
        if (age > timeout_ms) {
            stale |= bit;
        }
    }
    return stale;
}
