#include "icm42688_model.h"

#include "icm42688.h"

#include <string.h>

static uint8_t s_reg[128];
static uint32_t (*s_millis)(void);

static int32_t triangle(uint32_t time_ms, uint32_t period, int32_t amplitude, int32_t center)
{
    uint32_t half = period / 2u;
    uint32_t x = time_ms % period;
    int32_t rise = (x < half) ? (int32_t)x : (int32_t)(period - x);
    return center + (int32_t)(((int64_t)rise * amplitude) / (int32_t)half) - (amplitude / 2);
}

static void put_be16(int offset, int16_t value)
{
    uint16_t u = (uint16_t)value;
    s_reg[offset] = (uint8_t)(u >> 8);
    s_reg[offset + 1] = (uint8_t)(u & 0xFFu);
}

static void refresh(void)
{
    uint32_t now = (s_millis != NULL) ? s_millis() : 0u;
    /* milli-g and 0.1 dps, converted back to the ±4 g / ±500 dps codes. */
    int32_t ax_mg = triangle(now, 2000u, 80, 0);
    int32_t ay_mg = triangle(now, 2400u, 60, 0);
    int32_t az_mg = triangle(now, 5000u, 40, 1000);
    int32_t gx = triangle(now, 1800u, 30, 0);
    int32_t gy = triangle(now, 2200u, 20, 0);
    int32_t gz = triangle(now, 2600u, 15, 0);
    int16_t temp = (int16_t)triangle(now, 6000u, 200, 0);

    put_be16(0x1D, temp);
    put_be16(0x1F, (int16_t)((ax_mg * 8192) / 1000));
    put_be16(0x21, (int16_t)((ay_mg * 8192) / 1000));
    put_be16(0x23, (int16_t)((az_mg * 8192) / 1000));
    put_be16(0x25, (int16_t)((gx * 131) / 20));
    put_be16(0x27, (int16_t)((gy * 131) / 20));
    put_be16(0x29, (int16_t)((gz * 131) / 20));
}

void icm42688_model_init(uint32_t (*millis)(void))
{
    memset(s_reg, 0, sizeof s_reg);
    s_millis = millis;
    s_reg[0x75] = ICM42688_WHO_AM_I;
    refresh();
}

int icm42688_model_transfer(const uint8_t *tx, uint8_t *rx, size_t len)
{
    size_t i;
    uint8_t reg;
    int reading;

    if ((tx == NULL) || (rx == NULL) || (len == 0u)) {
        return -1;
    }

    reg = (uint8_t)(tx[0] & 0x7Fu);
    reading = (tx[0] & 0x80u) != 0u;
    rx[0] = 0u;

    if (reading && (reg <= 0x2Au)) {
        refresh();
    }

    for (i = 1; i < len; i++) {
        if (reg >= sizeof s_reg) {
            return -1;
        }
        if (reading) {
            rx[i] = s_reg[reg];
        } else {
            s_reg[reg] = tx[i];
            rx[i] = 0u;
        }
        reg++;
    }
    return 0;
}
