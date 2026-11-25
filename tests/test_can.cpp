#include "can_codec.h"
#include "can_sim.h"
#include "test_harness.h"

static int g_hw_fail;

static int hw_send(const can_frame_t *frame)
{
    (void)frame;
    return g_hw_fail;
}

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8);
}

int main(void)
{
    env_sample_t env = {};
    imu_sample_t imu = {};
    can_frame_t frame;
    can_frame_t imu_frames[2];
    can_frame_t bad;

    env.temp_centi_c = 2510;
    env.pressure_pa = 101325;
    env.humidity_centi_pct = 4800;
    env.time_ms = 0x1122;
    EXPECT(can_pack_env(&env, &frame) == 0);
    EXPECT(frame.id == CAN_ID_ENV);
    EXPECT(frame.dlc == 8);
    EXPECT((int16_t)le16(frame.data) == 2510);
    EXPECT(le16(frame.data + 2) == 1013u);
    EXPECT(le16(frame.data + 4) == 4800u);

    imu.ax_mg = 1;
    imu.ay_mg = -2;
    imu.az_mg = 1000;
    imu.gx_dps10 = 3;
    imu.gy_dps10 = 4;
    imu.gz_dps10 = -5;
    imu.temp_centi_c = 2600;
    imu.time_ms = 42;
    EXPECT(can_pack_imu(&imu, imu_frames) == 2);
    EXPECT(imu_frames[0].id == CAN_ID_IMU_ACC);
    EXPECT(imu_frames[1].id == CAN_ID_IMU_GYR);
    EXPECT((int16_t)le16(imu_frames[0].data + 4) == 1000);
    EXPECT((int16_t)le16(imu_frames[1].data + 4) == -5);
    EXPECT(le16(imu_frames[1].data + 6) == 42u);

    EXPECT(can_pack_health(4, 2, 0x01020304u, &frame) == 0);
    EXPECT(frame.id == CAN_ID_HEALTH);
    EXPECT(le16(frame.data) == 4u);
    EXPECT(le16(frame.data + 2) == 2u);
    EXPECT(frame.data[4] == 0x04u);
    EXPECT(frame.data[7] == 0x01u);

    can_sim_init(NULL);
    EXPECT(can_sim_transmit(&imu_frames[0]) == 0);
    EXPECT(can_sim_tx_count() == 1u);
    EXPECT(can_sim_last()->id == CAN_ID_IMU_ACC);

    can_sim_set_filter(CAN_ID_ENV, 0x7FFu);
    EXPECT(can_sim_transmit(&imu_frames[1]) != 0);
    EXPECT(can_sim_rejected() == 1u);
    EXPECT(can_sim_transmit(&frame) != 0);

    bad.dlc = 9;
    bad.id = 1;
    EXPECT(can_sim_transmit(&bad) != 0);

    can_sim_init(hw_send);
    g_hw_fail = -1;
    EXPECT(can_sim_transmit(&imu_frames[0]) != 0);
    EXPECT(can_sim_hw_errors() == 1u);
    g_hw_fail = 0;
    EXPECT(can_sim_transmit(&imu_frames[0]) == 0);

    EXPECT(can_pack_env(NULL, &frame) != 0);
    return g_test_fails == 0 ? 0 : 1;
}
