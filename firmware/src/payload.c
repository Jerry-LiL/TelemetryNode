#include "payload.h"

#include <string.h>

static void put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)(v >> 8);
}

static void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)(v >> 24);
}

static uint16_t get_u16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t get_u32(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

int payload_pack_env(uint8_t *dst, size_t cap, const env_sample_t *sample)
{
    if ((dst == NULL) || (sample == NULL) || (cap < PAYLOAD_ENV_LEN)) {
        return -1;
    }
    put_u32(dst, (uint32_t)sample->temp_centi_c);
    put_u32(dst + 4, sample->pressure_pa);
    put_u16(dst + 8, sample->humidity_centi_pct);
    put_u32(dst + 10, sample->time_ms);
    return PAYLOAD_ENV_LEN;
}

int payload_unpack_env(const uint8_t *src, size_t len, env_sample_t *sample)
{
    if ((src == NULL) || (sample == NULL) || (len < PAYLOAD_ENV_LEN)) {
        return -1;
    }
    sample->temp_centi_c = (int32_t)get_u32(src);
    sample->pressure_pa = get_u32(src + 4);
    sample->humidity_centi_pct = get_u16(src + 8);
    sample->time_ms = get_u32(src + 10);
    return 0;
}

int payload_pack_imu(uint8_t *dst, size_t cap, const imu_sample_t *sample)
{
    if ((dst == NULL) || (sample == NULL) || (cap < PAYLOAD_IMU_LEN)) {
        return -1;
    }
    put_u16(dst + 0, (uint16_t)sample->ax_mg);
    put_u16(dst + 2, (uint16_t)sample->ay_mg);
    put_u16(dst + 4, (uint16_t)sample->az_mg);
    put_u16(dst + 6, (uint16_t)sample->gx_dps10);
    put_u16(dst + 8, (uint16_t)sample->gy_dps10);
    put_u16(dst + 10, (uint16_t)sample->gz_dps10);
    put_u16(dst + 12, (uint16_t)sample->temp_centi_c);
    put_u32(dst + 14, sample->time_ms);
    return PAYLOAD_IMU_LEN;
}

int payload_unpack_imu(const uint8_t *src, size_t len, imu_sample_t *sample)
{
    if ((src == NULL) || (sample == NULL) || (len < PAYLOAD_IMU_LEN)) {
        return -1;
    }
    sample->ax_mg = (int16_t)get_u16(src + 0);
    sample->ay_mg = (int16_t)get_u16(src + 2);
    sample->az_mg = (int16_t)get_u16(src + 4);
    sample->gx_dps10 = (int16_t)get_u16(src + 6);
    sample->gy_dps10 = (int16_t)get_u16(src + 8);
    sample->gz_dps10 = (int16_t)get_u16(src + 10);
    sample->temp_centi_c = (int16_t)get_u16(src + 12);
    sample->time_ms = get_u32(src + 14);
    return 0;
}

int payload_pack_can(uint8_t *dst, size_t cap, const can_frame_view_t *frame)
{
    if ((dst == NULL) || (frame == NULL) || (cap < PAYLOAD_CAN_LEN) || (frame->dlc > 8u)) {
        return -1;
    }
    put_u16(dst, frame->id);
    dst[2] = frame->dlc;
    memset(dst + 3, 0, 8u);
    if (frame->dlc > 0u) {
        memcpy(dst + 3, frame->data, frame->dlc);
    }
    return PAYLOAD_CAN_LEN;
}

int payload_unpack_can(const uint8_t *src, size_t len, can_frame_view_t *frame)
{
    if ((src == NULL) || (frame == NULL) || (len < PAYLOAD_CAN_LEN) || (src[2] > 8u)) {
        return -1;
    }
    frame->id = get_u16(src);
    frame->dlc = src[2];
    memset(frame->data, 0, sizeof frame->data);
    memcpy(frame->data, src + 3, 8u);
    return 0;
}

int payload_pack_wdg(uint8_t *dst, size_t cap, const wdg_event_t *event)
{
    if ((dst == NULL) || (event == NULL) || (cap < PAYLOAD_WDG_LEN)) {
        return -1;
    }
    dst[0] = event->task_id;
    dst[1] = event->cause;
    put_u16(dst + 2, event->recovery_count);
    put_u32(dst + 4, event->stale_ms);
    return PAYLOAD_WDG_LEN;
}

int payload_unpack_wdg(const uint8_t *src, size_t len, wdg_event_t *event)
{
    if ((src == NULL) || (event == NULL) || (len < PAYLOAD_WDG_LEN)) {
        return -1;
    }
    event->task_id = src[0];
    event->cause = src[1];
    event->recovery_count = get_u16(src + 2);
    event->stale_ms = get_u32(src + 4);
    return 0;
}
