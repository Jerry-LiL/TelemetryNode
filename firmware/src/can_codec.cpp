#include "can_codec.h"

#include <cstdint>
#include <cstring>

namespace {

void put_le16(uint8_t *p, uint16_t value)
{
    p[0] = static_cast<uint8_t>(value & 0xFFu);
    p[1] = static_cast<uint8_t>(value >> 8);
}

/* One place that turns a sample into an 8-byte classic CAN payload. */
class Packetiser {
public:
    static can_frame_t environment(const env_sample_t &sample)
    {
        can_frame_t frame = {};
        int32_t temp = sample.temp_centi_c;
        if (temp > 32767) {
            temp = 32767;
        } else if (temp < -32768) {
            temp = -32768;
        }
        frame.id = CAN_ID_ENV;
        frame.dlc = 8;
        put_le16(frame.data + 0, static_cast<uint16_t>(static_cast<int16_t>(temp)));
        put_le16(frame.data + 2, static_cast<uint16_t>(sample.pressure_pa / 100u));
        put_le16(frame.data + 4, sample.humidity_centi_pct);
        put_le16(frame.data + 6, static_cast<uint16_t>(sample.time_ms));
        return frame;
    }

    static void imu(const imu_sample_t &sample, can_frame_t out[2])
    {
        out[0] = {};
        out[1] = {};
        out[0].id = CAN_ID_IMU_ACC;
        out[0].dlc = 8;
        put_le16(out[0].data + 0, static_cast<uint16_t>(sample.ax_mg));
        put_le16(out[0].data + 2, static_cast<uint16_t>(sample.ay_mg));
        put_le16(out[0].data + 4, static_cast<uint16_t>(sample.az_mg));
        put_le16(out[0].data + 6, static_cast<uint16_t>(sample.temp_centi_c));

        out[1].id = CAN_ID_IMU_GYR;
        out[1].dlc = 8;
        put_le16(out[1].data + 0, static_cast<uint16_t>(sample.gx_dps10));
        put_le16(out[1].data + 2, static_cast<uint16_t>(sample.gy_dps10));
        put_le16(out[1].data + 4, static_cast<uint16_t>(sample.gz_dps10));
        put_le16(out[1].data + 6, static_cast<uint16_t>(sample.time_ms));
    }

    static can_frame_t health(uint16_t boot_count, uint16_t recoveries, uint32_t uptime_ms)
    {
        can_frame_t frame = {};
        frame.id = CAN_ID_HEALTH;
        frame.dlc = 8;
        put_le16(frame.data + 0, boot_count);
        put_le16(frame.data + 2, recoveries);
        frame.data[4] = static_cast<uint8_t>(uptime_ms & 0xFFu);
        frame.data[5] = static_cast<uint8_t>((uptime_ms >> 8) & 0xFFu);
        frame.data[6] = static_cast<uint8_t>((uptime_ms >> 16) & 0xFFu);
        frame.data[7] = static_cast<uint8_t>(uptime_ms >> 24);
        return frame;
    }
};

}  // namespace

extern "C" int can_pack_env(const env_sample_t *sample, can_frame_t *out)
{
    if ((sample == nullptr) || (out == nullptr)) {
        return -1;
    }
    *out = Packetiser::environment(*sample);
    return 0;
}

extern "C" int can_pack_imu(const imu_sample_t *sample, can_frame_t out[2])
{
    if ((sample == nullptr) || (out == nullptr)) {
        return -1;
    }
    Packetiser::imu(*sample, out);
    return 2;
}

extern "C" int can_pack_health(uint16_t boot_count, uint16_t recoveries, uint32_t uptime_ms,
                               can_frame_t *out)
{
    if (out == nullptr) {
        return -1;
    }
    *out = Packetiser::health(boot_count, recoveries, uptime_ms);
    return 0;
}
