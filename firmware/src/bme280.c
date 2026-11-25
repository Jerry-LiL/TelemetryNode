#include "bme280.h"

#include "board.h"

#include <stddef.h>

#define REG_CALIB_TP   0x88u
#define REG_CHIP_ID    0xD0u
#define REG_RESET      0xE0u
#define REG_CALIB_H    0xE1u
#define REG_CTRL_HUM   0xF2u
#define REG_CTRL_MEAS  0xF4u
#define REG_CONFIG     0xF5u
#define REG_DATA       0xF7u

#define RESET_CMD      0xB6u
#define LEN_CALIB_TP   26u
#define LEN_CALIB_H    7u
#define LEN_DATA       8u

/* ctrl_hum oversampling x1, ctrl_meas temp x1 / press x1 / normal mode, filter off. */
#define CTRL_HUM_OS_1  0x01u
#define CTRL_MEAS_RUN  0x27u
#define CONFIG_T_SB    0x00u

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static int write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = value;
    return board_i2c_write(addr, buf, sizeof buf);
}

static int read_regs(uint8_t addr, uint8_t reg, uint8_t *dst, size_t len)
{
    return board_i2c_write_read(addr, &reg, 1u, dst, len);
}

static void parse_calib(bme280_t *dev, const uint8_t *tp, const uint8_t *h)
{
    dev->dig_t1 = le16(tp + 0);
    dev->dig_t2 = (int16_t)le16(tp + 2);
    dev->dig_t3 = (int16_t)le16(tp + 4);
    dev->dig_p1 = le16(tp + 6);
    dev->dig_p2 = (int16_t)le16(tp + 8);
    dev->dig_p3 = (int16_t)le16(tp + 10);
    dev->dig_p4 = (int16_t)le16(tp + 12);
    dev->dig_p5 = (int16_t)le16(tp + 14);
    dev->dig_p6 = (int16_t)le16(tp + 16);
    dev->dig_p7 = (int16_t)le16(tp + 18);
    dev->dig_p8 = (int16_t)le16(tp + 20);
    dev->dig_p9 = (int16_t)le16(tp + 22);
    dev->dig_h1 = tp[25];

    dev->dig_h2 = (int16_t)le16(h + 0);
    dev->dig_h3 = h[2];
    dev->dig_h4 = (int16_t)(((int16_t)(int8_t)h[3] << 4) | (int16_t)(h[4] & 0x0Fu));
    dev->dig_h5 = (int16_t)(((int16_t)(int8_t)h[5] << 4) | (int16_t)(h[4] >> 4));
    dev->dig_h6 = (int8_t)h[6];
}

int32_t bme280_comp_temp_centi(bme280_t *dev, int32_t adc_t)
{
    int32_t var1;
    int32_t var2;
    int32_t temperature;

    var1 = (adc_t / 8) - ((int32_t)dev->dig_t1 * 2);
    var1 = (var1 * (int32_t)dev->dig_t2) / 2048;
    var2 = (adc_t / 16) - (int32_t)dev->dig_t1;
    var2 = (((var2 * var2) / 4096) * (int32_t)dev->dig_t3) / 16384;
    dev->t_fine = var1 + var2;
    temperature = (dev->t_fine * 5 + 128) / 256;

    if (temperature < -4000) {
        temperature = -4000;
    } else if (temperature > 8500) {
        temperature = 8500;
    }
    return temperature;
}

uint32_t bme280_comp_press_pa(const bme280_t *dev, int32_t adc_p)
{
    int64_t var1;
    int64_t var2;
    int64_t var3;
    int64_t var4;
    int64_t scaled;

    var1 = (int64_t)dev->t_fine - 128000;
    var2 = var1 * var1 * (int64_t)dev->dig_p6;
    var2 = var2 + ((var1 * (int64_t)dev->dig_p5) * 131072);
    var2 = var2 + (((int64_t)dev->dig_p4) * 34359738368LL);
    var1 = ((var1 * var1 * (int64_t)dev->dig_p3) / 256) + (var1 * (int64_t)dev->dig_p2 * 4096);
    var3 = 140737488355328LL;
    var1 = (var3 + var1) * (int64_t)dev->dig_p1 / 8589934592LL;
    if (var1 == 0) {
        return 30000u;
    }

    var4 = 1048576 - (int64_t)adc_p;
    var4 = (((var4 * 2147483648LL) - var2) * 3125) / var1;
    var1 = (((int64_t)dev->dig_p9) * (var4 / 8192) * (var4 / 8192)) / 33554432;
    var2 = (((int64_t)dev->dig_p8) * var4) / 524288;
    var4 = ((var4 + var1 + var2) / 256) + (((int64_t)dev->dig_p7) * 16);
    scaled = ((var4 / 2) * 100) / 128;

    if (scaled < 3000000) {
        return 30000u;
    }
    if (scaled > 11000000) {
        return 110000u;
    }
    return (uint32_t)((scaled + 50) / 100);
}

uint32_t bme280_comp_hum_centi(const bme280_t *dev, int32_t adc_h)
{
    int32_t var1;
    int32_t var2;
    int32_t var3;
    int32_t var4;
    int32_t var5;
    int32_t q;

    var1 = dev->t_fine - 76800;
    var2 = adc_h * 16384;
    var3 = (int32_t)dev->dig_h4 * 1048576;
    var4 = (int32_t)dev->dig_h5 * var1;
    var5 = (((var2 - var3) - var4) + 16384) / 32768;
    var2 = (var1 * (int32_t)dev->dig_h6) / 1024;
    var3 = (var1 * (int32_t)dev->dig_h3) / 2048;
    var4 = ((var2 * (var3 + 32768)) / 1024) + 2097152;
    var2 = ((var4 * (int32_t)dev->dig_h2) + 8192) / 16384;
    var3 = var5 * var2;
    var4 = ((var3 / 32768) * (var3 / 32768)) / 128;
    var5 = var3 - ((var4 * (int32_t)dev->dig_h1) / 16);
    if (var5 < 0) {
        var5 = 0;
    } else if (var5 > 419430400) {
        var5 = 419430400;
    }
    q = var5 / 4096;
    if (q > 102400) {
        q = 102400;
    }
    return (uint32_t)((q * 100 + 512) / 1024);
}

int bme280_init(bme280_t *dev, uint8_t addr, tn_delay_ms_fn delay_ms)
{
    uint8_t id = 0;
    uint8_t tp[LEN_CALIB_TP];
    uint8_t hum[LEN_CALIB_H];

    if (dev == NULL) {
        return -1;
    }
    dev->ready = 0;
    dev->addr = addr;
    dev->t_fine = 0;

    if (write_reg(addr, REG_RESET, RESET_CMD) != 0) {
        return -1;
    }
    if (delay_ms != NULL) {
        delay_ms(2u);
    }
    if ((read_regs(addr, REG_CHIP_ID, &id, 1u) != 0) || (id != BME280_CHIP_ID)) {
        return -2;
    }
    if (read_regs(addr, REG_CALIB_TP, tp, sizeof tp) != 0) {
        return -1;
    }
    if (read_regs(addr, REG_CALIB_H, hum, sizeof hum) != 0) {
        return -1;
    }
    parse_calib(dev, tp, hum);
    if (dev->dig_t1 == 0u || dev->dig_p1 == 0u) {
        return -3;
    }

    /* Humidity register is latched only when ctrl_meas is written afterwards. */
    if (write_reg(addr, REG_CTRL_HUM, CTRL_HUM_OS_1) != 0) {
        return -1;
    }
    if (write_reg(addr, REG_CONFIG, CONFIG_T_SB) != 0) {
        return -1;
    }
    if (write_reg(addr, REG_CTRL_MEAS, CTRL_MEAS_RUN) != 0) {
        return -1;
    }
    dev->ready = 1;
    return 0;
}

int bme280_read(bme280_t *dev, env_sample_t *out)
{
    uint8_t raw[LEN_DATA];
    int32_t adc_p;
    int32_t adc_t;
    int32_t adc_h;

    if ((dev == NULL) || (out == NULL) || (dev->ready == 0)) {
        return -1;
    }
    if (read_regs(dev->addr, REG_DATA, raw, sizeof raw) != 0) {
        return -1;
    }

    adc_p = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | ((int32_t)raw[2] >> 4);
    adc_t = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | ((int32_t)raw[5] >> 4);
    adc_h = ((int32_t)raw[6] << 8) | (int32_t)raw[7];

    out->temp_centi_c = bme280_comp_temp_centi(dev, adc_t);
    out->pressure_pa = bme280_comp_press_pa(dev, adc_p);
    out->humidity_centi_pct = (uint16_t)bme280_comp_hum_centi(dev, adc_h);
    out->time_ms = 0;
    return 0;
}
