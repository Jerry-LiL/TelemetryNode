#include "bme280.h"
#include "bme280_model.h"
#include "icm42688.h"
#include "icm42688_model.h"
#include "test_harness.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static uint32_t g_now;

static uint32_t fake_millis(void)
{
    return g_now;
}

int board_i2c_write(uint8_t addr, const uint8_t *data, size_t len)
{
    return bme280_model_write(addr, data, len);
}

int board_i2c_write_read(uint8_t addr, const uint8_t *wdata, size_t wlen,
                         uint8_t *rdata, size_t rlen)
{
    return bme280_model_write_read(addr, wdata, wlen, rdata, rlen);
}

int board_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len)
{
    return icm42688_model_transfer(tx, rx, len);
}

static double ref_temp(const bme280_t *dev, int32_t adc, int32_t *t_fine)
{
    double var1 = ((double)adc / 16384.0) - ((double)dev->dig_t1 / 1024.0);
    double var2;
    var1 *= (double)dev->dig_t2;
    var2 = ((double)adc / 131072.0) - ((double)dev->dig_t1 / 8192.0);
    var2 = var2 * var2 * (double)dev->dig_t3;
    *t_fine = (int32_t)(var1 + var2);
    return (var1 + var2) / 5120.0;
}

static double ref_press(const bme280_t *dev, int32_t adc, int32_t t_fine)
{
    double var1 = ((double)t_fine / 2.0) - 64000.0;
    double var2 = var1 * var1 * (double)dev->dig_p6 / 32768.0;
    double var3;
    double pressure;
    var2 = var2 + var1 * (double)dev->dig_p5 * 2.0;
    var2 = (var2 / 4.0) + ((double)dev->dig_p4 * 65536.0);
    var3 = (double)dev->dig_p3 * var1 * var1 / 524288.0;
    var1 = (var3 + ((double)dev->dig_p2 * var1)) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * (double)dev->dig_p1;
    pressure = 1048576.0 - (double)adc;
    pressure = (pressure - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = (double)dev->dig_p9 * pressure * pressure / 2147483648.0;
    var2 = pressure * (double)dev->dig_p8 / 32768.0;
    pressure = pressure + (var1 + var2 + (double)dev->dig_p7) / 16.0;
    return pressure;
}

static void load_typical_calib(bme280_t *dev)
{
    memset(dev, 0, sizeof *dev);
    dev->dig_t1 = 28176;
    dev->dig_t2 = 26234;
    dev->dig_t3 = 50;
    dev->dig_p1 = 36672;
    dev->dig_p2 = -10632;
    dev->dig_p3 = 3024;
    dev->dig_p4 = 11016;
    dev->dig_p5 = -234;
    dev->dig_p6 = -7;
    dev->dig_p7 = 15500;
    dev->dig_p8 = -14600;
    dev->dig_p9 = 6000;
    dev->dig_h1 = 75;
    dev->dig_h2 = 360;
    dev->dig_h3 = 0;
    dev->dig_h4 = 340;
    dev->dig_h5 = 0;
    dev->dig_h6 = 30;
}

int main(void)
{
    bme280_t calib;
    bme280_t dev;
    env_sample_t env;
    icm42688_t imu_dev;
    imu_sample_t imu;
    int32_t t_fine = 0;
    int32_t adc_t = 519888;
    double t_ref;
    int32_t t_int;
    int32_t want_t;
    uint32_t want_p;
    uint16_t want_h;
    int32_t delta_t;

    load_typical_calib(&calib);
    t_ref = ref_temp(&calib, adc_t, &t_fine);
    t_int = bme280_comp_temp_centi(&calib, adc_t);
    EXPECT(t_int > -4000 && t_int < 8500);
    EXPECT(fabs((t_int / 100.0) - t_ref) < 0.05);

    (void)bme280_comp_temp_centi(&calib, adc_t);
    {
        double p_ref = ref_press(&calib, 415000, calib.t_fine);
        uint32_t p_int = bme280_comp_press_pa(&calib, 415000);
        EXPECT(p_int > 30000u && p_int < 110000u);
        EXPECT(fabs((double)p_int - p_ref) < 5.0);
    }

    EXPECT(icm42688_accel_mg(8192) == 1000);
    EXPECT(icm42688_accel_mg(-8192) == -1000);
    EXPECT(icm42688_gyro_dps10(131) == 20);

    g_now = 2500;
    bme280_model_init(fake_millis);
    bme280_model_environment(g_now, &want_t, &want_p, &want_h);
    EXPECT(bme280_init(&dev, BME280_I2C_ADDR_SDO_LOW, NULL) == 0);
    EXPECT(dev.dig_t1 == 28176);
    EXPECT(dev.dig_h4 == 340);
    EXPECT(dev.dig_h2 == 360);
    EXPECT(bme280_read(&dev, &env) == 0);
    delta_t = env.temp_centi_c - want_t;
    if (delta_t < 0) {
        delta_t = -delta_t;
    }
    EXPECT(delta_t <= 2);
    EXPECT(env.pressure_pa + 3 >= want_p && env.pressure_pa <= want_p + 3);
    EXPECT(env.humidity_centi_pct + 50 >= want_h && env.humidity_centi_pct <= want_h + 50);

    icm42688_model_init(fake_millis);
    EXPECT(icm42688_init(&imu_dev, NULL) == 0);
    EXPECT(icm42688_read(&imu_dev, &imu) == 0);
    EXPECT(imu.az_mg > 900 && imu.az_mg < 1100);
    EXPECT(imu.ax_mg > -200 && imu.ax_mg < 200);

    EXPECT(bme280_init(&dev, 0x10, NULL) != 0);
    return g_test_fails == 0 ? 0 : 1;
}
