#include "icm42688.h"

#include "board.h"

#include <stddef.h>

#define REG_DEVICE_CONFIG  0x11u
#define REG_TEMP_DATA1     0x1Du
#define REG_ACCEL_DATA_X1  0x1Fu
#define REG_PWR_MGMT0      0x4Eu
#define REG_GYRO_CONFIG0   0x4Fu
#define REG_ACCEL_CONFIG0  0x50u
#define REG_WHO_AM_I       0x75u
#define REG_BANK_SEL       0x76u

#define SPI_READ           0x80u
#define SOFT_RESET         0x01u
/* Gyro and accel low-noise, both sensors on. */
#define PWR_LN_BOTH        0x0Fu
/* FS_SEL = 010 (±500 dps or ±4 g) and ODR = 50 Hz. */
#define FS_500_OR_4G_50HZ  0x49u

static int spi_read(uint8_t reg, uint8_t *dst, size_t len)
{
    uint8_t tx[16];
    uint8_t rx[16];
    size_t i;

    if ((len + 1u) > sizeof tx) {
        return -1;
    }
    tx[0] = (uint8_t)(reg | SPI_READ);
    for (i = 1; i <= len; i++) {
        tx[i] = 0u;
    }
    if (board_spi_transfer(tx, rx, len + 1u) != 0) {
        return -1;
    }
    for (i = 0; i < len; i++) {
        dst[i] = rx[i + 1u];
    }
    return 0;
}

static int spi_write(uint8_t reg, uint8_t value)
{
    uint8_t tx[2];
    uint8_t rx[2];
    tx[0] = reg;
    tx[1] = value;
    return board_spi_transfer(tx, rx, sizeof tx);
}

static int16_t be16(const uint8_t *p)
{
    return (int16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

int16_t icm42688_accel_mg(int16_t raw)
{
    return (int16_t)(((int32_t)raw * 1000) / 8192);
}

int16_t icm42688_gyro_dps10(int16_t raw)
{
    /* 10 / 65.5 = 20 / 131. */
    return (int16_t)(((int32_t)raw * 20) / 131);
}

int icm42688_init(icm42688_t *dev, tn_delay_ms_fn delay_ms)
{
    uint8_t who = 0;

    if (dev == NULL) {
        return -1;
    }
    dev->ready = 0;

    if (spi_write(REG_BANK_SEL, 0x00u) != 0) {
        return -1;
    }
    if (spi_read(REG_WHO_AM_I, &who, 1u) != 0) {
        return -1;
    }
    if (who != ICM42688_WHO_AM_I) {
        return -2;
    }
    if (spi_write(REG_DEVICE_CONFIG, SOFT_RESET) != 0) {
        return -1;
    }
    if (delay_ms != NULL) {
        delay_ms(2u);
    }
    if (spi_write(REG_PWR_MGMT0, PWR_LN_BOTH) != 0) {
        return -1;
    }
    if (spi_write(REG_GYRO_CONFIG0, FS_500_OR_4G_50HZ) != 0) {
        return -1;
    }
    if (spi_write(REG_ACCEL_CONFIG0, FS_500_OR_4G_50HZ) != 0) {
        return -1;
    }
    dev->ready = 1;
    return 0;
}

int icm42688_read(icm42688_t *dev, imu_sample_t *out)
{
    uint8_t raw[14];
    int16_t temp_raw;

    if ((dev == NULL) || (out == NULL) || (dev->ready == 0)) {
        return -1;
    }
    /* TEMP_DATA1 through GYRO_DATA_Z0. Accel X1 sits two bytes after the temperature. */
    if (spi_read(REG_TEMP_DATA1, raw, sizeof raw) != 0) {
        return -1;
    }

    temp_raw = be16(raw + 0);
    out->temp_centi_c = (int16_t)(((int32_t)temp_raw * 100) / 132 + 2500);
    out->ax_mg = icm42688_accel_mg(be16(raw + 2));
    out->ay_mg = icm42688_accel_mg(be16(raw + 4));
    out->az_mg = icm42688_accel_mg(be16(raw + 6));
    out->gx_dps10 = icm42688_gyro_dps10(be16(raw + 8));
    out->gy_dps10 = icm42688_gyro_dps10(be16(raw + 10));
    out->gz_dps10 = icm42688_gyro_dps10(be16(raw + 12));
    out->time_ms = 0;
    return 0;
}
