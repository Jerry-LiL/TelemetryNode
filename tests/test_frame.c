#include "frame.h"
#include "payload.h"
#include "test_harness.h"
#include "wdg.h"

#include <string.h>

int main(void)
{
    frame_t in;
    frame_t out;
    uint8_t wire[64];
    int n;
    env_sample_t env;
    env_sample_t env_out;
    imu_sample_t imu;
    imu_sample_t imu_out;
    wdg_event_t wdg;
    wdg_event_t wdg_out;
    uint8_t payload[32];

    memset(&in, 0, sizeof in);
    in.type = FRAME_LOG;
    in.seq = 0x1234;
    in.len = 4;
    memcpy(in.payload, "pong", 4);

    n = frame_encode(wire, sizeof wire, &in, 0);
    EXPECT(n == (int)frame_wire_size(4));
    EXPECT(frame_decode(wire, (size_t)n, &out) == 0);
    EXPECT(out.type == FRAME_LOG);
    EXPECT(out.seq == 0x1234);
    EXPECT(out.len == 4);
    EXPECT(memcmp(out.payload, "pong", 4) == 0);

    n = frame_encode(wire, sizeof wire, &in, 1);
    EXPECT(frame_decode(wire, (size_t)n, &out) == -3);
    wire[0] = 0x00;
    EXPECT(frame_decode(wire, (size_t)n, &out) == -1);

    memset(&env, 0, sizeof env);
    env.temp_centi_c = -1250;
    env.pressure_pa = 101325;
    env.humidity_centi_pct = 4820;
    env.time_ms = 0x01020304;
    EXPECT(payload_pack_env(payload, sizeof payload, &env) == PAYLOAD_ENV_LEN);
    EXPECT(payload_unpack_env(payload, PAYLOAD_ENV_LEN, &env_out) == 0);
    EXPECT(env_out.temp_centi_c == -1250);
    EXPECT(env_out.pressure_pa == 101325u);
    EXPECT(env_out.humidity_centi_pct == 4820u);
    EXPECT(env_out.time_ms == 0x01020304u);

    memset(&imu, 0, sizeof imu);
    imu.ax_mg = -10;
    imu.ay_mg = 20;
    imu.az_mg = 1000;
    imu.gx_dps10 = -5;
    imu.gy_dps10 = 6;
    imu.gz_dps10 = 7;
    imu.temp_centi_c = 2510;
    imu.time_ms = 99;
    EXPECT(payload_pack_imu(payload, sizeof payload, &imu) == PAYLOAD_IMU_LEN);
    EXPECT(payload_unpack_imu(payload, PAYLOAD_IMU_LEN, &imu_out) == 0);
    EXPECT(imu_out.ax_mg == -10);
    EXPECT(imu_out.az_mg == 1000);
    EXPECT(imu_out.gz_dps10 == 7);
    EXPECT(imu_out.temp_centi_c == 2510);
    EXPECT(imu_out.time_ms == 99u);

    wdg.task_id = TN_TASK_ENV;
    wdg.cause = WDG_CAUSE_STALL;
    wdg.recovery_count = 3;
    wdg.stale_ms = 450;
    EXPECT(payload_pack_wdg(payload, sizeof payload, &wdg) == PAYLOAD_WDG_LEN);
    EXPECT(payload_unpack_wdg(payload, PAYLOAD_WDG_LEN, &wdg_out) == 0);
    EXPECT(wdg_out.task_id == TN_TASK_ENV);
    EXPECT(wdg_out.cause == WDG_CAUSE_STALL);
    EXPECT(wdg_out.recovery_count == 3u);
    EXPECT(wdg_out.stale_ms == 450u);

    return g_test_fails == 0 ? 0 : 1;
}
