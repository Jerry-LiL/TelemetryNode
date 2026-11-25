#ifndef TELEMETRY_BME280_H
#define TELEMETRY_BME280_H

#include "sample.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BME280_I2C_ADDR_SDO_LOW  0x76u
#define BME280_CHIP_ID           0x60u

typedef struct {
    uint16_t dig_t1;
    int16_t dig_t2;
    int16_t dig_t3;
    uint16_t dig_p1;
    int16_t dig_p2;
    int16_t dig_p3;
    int16_t dig_p4;
    int16_t dig_p5;
    int16_t dig_p6;
    int16_t dig_p7;
    int16_t dig_p8;
    int16_t dig_p9;
    uint8_t dig_h1;
    int16_t dig_h2;
    uint8_t dig_h3;
    int16_t dig_h4;
    int16_t dig_h5;
    int8_t dig_h6;
    int32_t t_fine;
    uint8_t addr;
    int ready;
} bme280_t;

typedef void (*tn_delay_ms_fn)(uint32_t ms);

int bme280_init(bme280_t *dev, uint8_t addr, tn_delay_ms_fn delay_ms);
int bme280_read(bme280_t *dev, env_sample_t *out);

/* Integer compensation from the BME280 datasheet (Bosch 64-bit pressure path). */
int32_t bme280_comp_temp_centi(bme280_t *dev, int32_t adc_t);
uint32_t bme280_comp_press_pa(const bme280_t *dev, int32_t adc_p);
uint32_t bme280_comp_hum_centi(const bme280_t *dev, int32_t adc_h);

#ifdef __cplusplus
}
#endif

#endif
