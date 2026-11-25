#ifndef TELEMETRY_PAYLOAD_H
#define TELEMETRY_PAYLOAD_H

#include "frame.h"
#include "sample.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t id;
    uint8_t dlc;
    uint8_t data[8];
} can_frame_view_t;

typedef struct {
    uint8_t task_id;
    uint8_t cause;
    uint16_t recovery_count;
    uint32_t stale_ms;
} wdg_event_t;

int payload_pack_env(uint8_t *dst, size_t cap, const env_sample_t *sample);
int payload_unpack_env(const uint8_t *src, size_t len, env_sample_t *sample);

int payload_pack_imu(uint8_t *dst, size_t cap, const imu_sample_t *sample);
int payload_unpack_imu(const uint8_t *src, size_t len, imu_sample_t *sample);

int payload_pack_can(uint8_t *dst, size_t cap, const can_frame_view_t *frame);
int payload_unpack_can(const uint8_t *src, size_t len, can_frame_view_t *frame);

int payload_pack_wdg(uint8_t *dst, size_t cap, const wdg_event_t *event);
int payload_unpack_wdg(const uint8_t *src, size_t len, wdg_event_t *event);

#ifdef __cplusplus
}
#endif

#endif
