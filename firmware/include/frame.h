#ifndef TELEMETRY_FRAME_H
#define TELEMETRY_FRAME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FRAME_SOF0          0xAAu
#define FRAME_SOF1          0x55u
#define FRAME_VERSION       1u
#define FRAME_MAX_PAYLOAD   48u

enum {
    FRAME_ENV = 0x01,
    FRAME_IMU = 0x02,
    FRAME_CAN = 0x03,
    FRAME_WDG = 0x10,
    FRAME_LOG = 0x11
};

enum {
    PAYLOAD_ENV_LEN = 14,
    PAYLOAD_IMU_LEN = 18,
    PAYLOAD_CAN_LEN = 11,
    PAYLOAD_WDG_LEN = 8
};

typedef struct {
    uint8_t type;
    uint16_t seq;
    uint8_t len;
    uint8_t payload[FRAME_MAX_PAYLOAD];
} frame_t;

/* Bytes on the wire for a payload of the given length, including SOF and CRC. */
size_t frame_wire_size(uint8_t payload_len);

/*
 * Encode one frame. When corrupt is non-zero the first payload byte is
 * flipped after the CRC is appended, so a peer must reject the frame.
 * Returns the number of bytes written, or -1.
 */
int frame_encode(uint8_t *out, size_t cap, const frame_t *frame, int corrupt);

/* Decode one complete frame. 0 on success, -1 bad header, -2 bad length, -3 bad CRC. */
int frame_decode(const uint8_t *in, size_t len, frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif
