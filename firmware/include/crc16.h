#ifndef TELEMETRY_CRC16_H
#define TELEMETRY_CRC16_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection, xorout 0. */
uint16_t crc16_ccitt(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
