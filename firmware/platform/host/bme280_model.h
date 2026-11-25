#ifndef TELEMETRY_BME280_MODEL_H
#define TELEMETRY_BME280_MODEL_H

#include <stddef.h>
#include <stdint.h>

void bme280_model_init(uint32_t (*millis)(void));
void bme280_model_environment(uint32_t time_ms, int32_t *temp_centi_c,
                              uint32_t *pressure_pa, uint16_t *humidity_centi_pct);

int bme280_model_write(uint8_t addr, const uint8_t *data, size_t len);
int bme280_model_write_read(uint8_t addr, const uint8_t *wdata, size_t wlen,
                            uint8_t *rdata, size_t rlen);

#endif
