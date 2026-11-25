#ifndef TELEMETRY_SAMPLE_H
#define TELEMETRY_SAMPLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Engineering units shared by the drivers, the UART payload, and CAN. */
typedef struct {
    int32_t temp_centi_c;
    uint32_t pressure_pa;
    uint16_t humidity_centi_pct;
    uint32_t time_ms;
} env_sample_t;

typedef struct {
    int16_t ax_mg;
    int16_t ay_mg;
    int16_t az_mg;
    int16_t gx_dps10;
    int16_t gy_dps10;
    int16_t gz_dps10;
    int16_t temp_centi_c;
    uint32_t time_ms;
} imu_sample_t;

#ifdef __cplusplus
}
#endif

#endif
