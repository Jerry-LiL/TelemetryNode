#ifndef TELEMETRY_CAN_CODEC_H
#define TELEMETRY_CAN_CODEC_H

#include "sample.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAN_ID_ENV      0x180u
#define CAN_ID_IMU_ACC  0x280u
#define CAN_ID_IMU_GYR  0x281u
#define CAN_ID_HEALTH   0x300u

typedef struct {
    uint16_t id;
    uint8_t dlc;
    uint8_t data[8];
} can_frame_t;

/* Classic CAN 2.0 data frames, 11-bit identifiers, DLC <= 8. */
int can_pack_env(const env_sample_t *sample, can_frame_t *out);
int can_pack_imu(const imu_sample_t *sample, can_frame_t out[2]);
int can_pack_health(uint16_t boot_count, uint16_t recoveries, uint32_t uptime_ms,
                    can_frame_t *out);

#ifdef __cplusplus
}
#endif

#endif
