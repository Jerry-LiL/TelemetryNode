#include "bme280_model.h"

#include "bme280.h"

#include <string.h>

#define REG_DATA 0xF7u

static uint8_t s_reg[256];
static uint8_t s_ptr;
static uint32_t (*s_millis)(void);
static bme280_t s_calib;

static void put_le16(int offset, uint16_t value)
{
    s_reg[offset] = (uint8_t)(value & 0xFFu);
    s_reg[offset + 1] = (uint8_t)(value >> 8);
}

static int32_t triangle(uint32_t time_ms, uint32_t period, int32_t amplitude, int32_t center)
{
    uint32_t half = period / 2u;
    uint32_t x = time_ms % period;
    int32_t rise = (x < half) ? (int32_t)x : (int32_t)(period - x);
    return center + (int32_t)(((int64_t)rise * amplitude) / (int32_t)half) - (amplitude / 2);
}

static int32_t search_temp(int32_t target)
{
    int32_t lo = 0;
    int32_t hi = 0xFFFFF;

    while (lo < hi) {
        int32_t mid = lo + ((hi - lo) / 2);
        int32_t got = bme280_comp_temp_centi(&s_calib, mid);
        if (got < target) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

static int32_t search_press(uint32_t target)
{
    int32_t lo = 0;
    int32_t hi = 0xFFFFF;

    while (lo < hi) {
        int32_t mid = lo + ((hi - lo) / 2);
        uint32_t got = bme280_comp_press_pa(&s_calib, mid);
        /* Larger raw pressure codes produce a lower Pascal reading. */
        if (got > target) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

static int32_t search_hum(uint32_t target)
{
    int32_t lo = 0;
    int32_t hi = 0xFFFF;

    while (lo < hi) {
        int32_t mid = lo + ((hi - lo) / 2);
        uint32_t got = bme280_comp_hum_centi(&s_calib, mid);
        if (got < target) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

static void store_adc(int32_t adc_p, int32_t adc_t, int32_t adc_h)
{
    s_reg[0xF7] = (uint8_t)((adc_p >> 12) & 0xFF);
    s_reg[0xF8] = (uint8_t)((adc_p >> 4) & 0xFF);
    s_reg[0xF9] = (uint8_t)((adc_p << 4) & 0xF0);
    s_reg[0xFA] = (uint8_t)((adc_t >> 12) & 0xFF);
    s_reg[0xFB] = (uint8_t)((adc_t >> 4) & 0xFF);
    s_reg[0xFC] = (uint8_t)((adc_t << 4) & 0xF0);
    s_reg[0xFD] = (uint8_t)((adc_h >> 8) & 0xFF);
    s_reg[0xFE] = (uint8_t)(adc_h & 0xFF);
}

static void refresh_data(void)
{
    int32_t temp;
    uint32_t pressure;
    uint16_t humidity;
    uint32_t now = (s_millis != NULL) ? s_millis() : 0u;
    int32_t adc_t;
    int32_t adc_p;
    int32_t adc_h;

    bme280_model_environment(now, &temp, &pressure, &humidity);
    adc_t = search_temp(temp);
    /* Temperature search leaves t_fine matching adc_t, which pressure and humidity need. */
    (void)bme280_comp_temp_centi(&s_calib, adc_t);
    adc_p = search_press(pressure);
    adc_h = search_hum(humidity);
    store_adc(adc_p, adc_t, adc_h);
}

void bme280_model_environment(uint32_t time_ms, int32_t *temp_centi_c,
                              uint32_t *pressure_pa, uint16_t *humidity_centi_pct)
{
    if (temp_centi_c != NULL) {
        *temp_centi_c = triangle(time_ms, 8000u, 600, 2200);
    }
    if (pressure_pa != NULL) {
        *pressure_pa = (uint32_t)triangle(time_ms, 10000u, 80, 101325);
    }
    if (humidity_centi_pct != NULL) {
        *humidity_centi_pct = (uint16_t)triangle(time_ms, 9000u, 1600, 4800);
    }
}

void bme280_model_init(uint32_t (*millis)(void))
{
    int16_t h4 = 340;
    int16_t h5 = 0;

    memset(s_reg, 0, sizeof s_reg);
    s_millis = millis;
    s_ptr = 0;

    memset(&s_calib, 0, sizeof s_calib);
    s_calib.dig_t1 = 28176;
    s_calib.dig_t2 = 26234;
    s_calib.dig_t3 = 50;
    s_calib.dig_p1 = 36672;
    s_calib.dig_p2 = -10632;
    s_calib.dig_p3 = 3024;
    s_calib.dig_p4 = 11016;
    s_calib.dig_p5 = -234;
    s_calib.dig_p6 = -7;
    s_calib.dig_p7 = 15500;
    s_calib.dig_p8 = -14600;
    s_calib.dig_p9 = 6000;
    s_calib.dig_h1 = 75;
    s_calib.dig_h2 = 360;
    s_calib.dig_h3 = 0;
    s_calib.dig_h4 = h4;
    s_calib.dig_h5 = h5;
    s_calib.dig_h6 = 30;

    s_reg[0xD0] = BME280_CHIP_ID;
    put_le16(0x88, s_calib.dig_t1);
    put_le16(0x8A, (uint16_t)s_calib.dig_t2);
    put_le16(0x8C, (uint16_t)s_calib.dig_t3);
    put_le16(0x8E, s_calib.dig_p1);
    put_le16(0x90, (uint16_t)s_calib.dig_p2);
    put_le16(0x92, (uint16_t)s_calib.dig_p3);
    put_le16(0x94, (uint16_t)s_calib.dig_p4);
    put_le16(0x96, (uint16_t)s_calib.dig_p5);
    put_le16(0x98, (uint16_t)s_calib.dig_p6);
    put_le16(0x9A, (uint16_t)s_calib.dig_p7);
    put_le16(0x9C, (uint16_t)s_calib.dig_p8);
    put_le16(0x9E, (uint16_t)s_calib.dig_p9);
    s_reg[0xA1] = s_calib.dig_h1;

    put_le16(0xE1, (uint16_t)s_calib.dig_h2);
    s_reg[0xE3] = s_calib.dig_h3;
    s_reg[0xE4] = (uint8_t)((h4 >> 4) & 0xFF);
    s_reg[0xE5] = (uint8_t)(((h4 & 0x0F) << 0) | ((h5 & 0x0F) << 4));
    s_reg[0xE6] = (uint8_t)((h5 >> 4) & 0xFF);
    s_reg[0xE7] = (uint8_t)s_calib.dig_h6;

    refresh_data();
}

static int accept(uint8_t addr)
{
    return addr == BME280_I2C_ADDR_SDO_LOW;
}

int bme280_model_write(uint8_t addr, const uint8_t *data, size_t len)
{
    size_t i;

    if (!accept(addr) || (data == NULL) || (len == 0u)) {
        return -1;
    }
    s_ptr = data[0];
    for (i = 1; i < len; i++) {
        s_reg[s_ptr] = data[i];
        s_ptr++;
    }
    return 0;
}

int bme280_model_write_read(uint8_t addr, const uint8_t *wdata, size_t wlen,
                            uint8_t *rdata, size_t rlen)
{
    size_t i;

    if (!accept(addr) || (wdata == NULL) || (wlen == 0u) || (rdata == NULL)) {
        return -1;
    }
    s_ptr = wdata[0];
    if ((s_ptr <= REG_DATA) && ((uint16_t)s_ptr + rlen > REG_DATA)) {
        refresh_data();
    }
    for (i = 0; i < rlen; i++) {
        rdata[i] = s_reg[s_ptr];
        s_ptr++;
    }
    return 0;
}
