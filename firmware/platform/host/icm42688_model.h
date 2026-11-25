#ifndef TELEMETRY_ICM42688_MODEL_H
#define TELEMETRY_ICM42688_MODEL_H

#include <stddef.h>
#include <stdint.h>

void icm42688_model_init(uint32_t (*millis)(void));
int icm42688_model_transfer(const uint8_t *tx, uint8_t *rx, size_t len);

#endif
