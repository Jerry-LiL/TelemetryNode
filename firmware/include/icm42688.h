#ifndef TELEMETRY_ICM42688_H
#define TELEMETRY_ICM42688_H

#include "bme280.h"
#include "sample.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ICM42688_WHO_AM_I  0x47u

typedef struct {
    int ready;
} icm42688_t;

int icm42688_init(icm42688_t *dev, tn_delay_ms_fn delay_ms);
int icm42688_read(icm42688_t *dev, imu_sample_t *out);

/* ±4 g -> 8192 LSB/g. ±500 dps -> 65.5 LSB/dps, reported in 0.1 dps. */
int16_t icm42688_accel_mg(int16_t raw);
int16_t icm42688_gyro_dps10(int16_t raw);

#ifdef __cplusplus
}
#endif

#endif
