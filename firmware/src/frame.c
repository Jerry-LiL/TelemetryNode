#include "frame.h"

#include "crc16.h"

#include <string.h>

size_t frame_wire_size(uint8_t payload_len)
{
    return (size_t)payload_len + 9u;
}

int frame_encode(uint8_t *out, size_t cap, const frame_t *frame, int corrupt)
{
    size_t total;
    uint16_t crc;

    if ((out == NULL) || (frame == NULL) || (frame->len > FRAME_MAX_PAYLOAD)) {
        return -1;
    }

    total = frame_wire_size(frame->len);
    if (cap < total) {
        return -1;
    }

    out[0] = FRAME_SOF0;
    out[1] = FRAME_SOF1;
    out[2] = FRAME_VERSION;
    out[3] = frame->type;
    out[4] = (uint8_t)(frame->seq & 0xFFu);
    out[5] = (uint8_t)(frame->seq >> 8);
    out[6] = frame->len;
    if (frame->len > 0u) {
        memcpy(&out[7], frame->payload, frame->len);
    }

    crc = crc16_ccitt(&out[2], (size_t)frame->len + 5u);
    out[7u + frame->len] = (uint8_t)(crc & 0xFFu);
    out[8u + frame->len] = (uint8_t)(crc >> 8);

    if ((corrupt != 0) && (frame->len > 0u)) {
        out[7] ^= 0xFFu;
    }

    return (int)total;
}

int frame_decode(const uint8_t *in, size_t len, frame_t *frame)
{
    uint8_t payload_len;
    size_t total;
    uint16_t expect;
    uint16_t got;

    if ((in == NULL) || (frame == NULL) || (len < 9u)) {
        return -1;
    }
    if ((in[0] != FRAME_SOF0) || (in[1] != FRAME_SOF1) || (in[2] != FRAME_VERSION)) {
        return -1;
    }

    payload_len = in[6];
    if (payload_len > FRAME_MAX_PAYLOAD) {
        return -2;
    }
    total = frame_wire_size(payload_len);
    if (len < total) {
        return -2;
    }

    expect = crc16_ccitt(&in[2], (size_t)payload_len + 5u);
    got = (uint16_t)in[7u + payload_len] | ((uint16_t)in[8u + payload_len] << 8);
    if (got != expect) {
        return -3;
    }

    frame->type = in[3];
    frame->seq = (uint16_t)in[4] | ((uint16_t)in[5] << 8);
    frame->len = payload_len;
    if (payload_len > 0u) {
        memcpy(frame->payload, &in[7], payload_len);
    }
    return 0;
}
