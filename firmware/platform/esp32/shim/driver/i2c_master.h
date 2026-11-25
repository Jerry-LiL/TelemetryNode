#ifndef ESP32_SHIM_I2C_MASTER_H
#define ESP32_SHIM_I2C_MASTER_H

#include <stddef.h>
#include <stdint.h>

#ifndef ESP_OK
typedef int esp_err_t;
#define ESP_OK 0
#endif
#define I2C_NUM_0 0
#define I2C_CLK_SRC_DEFAULT 0
#define I2C_ADDR_BIT_LEN_7 7

typedef struct i2c_master_bus_t *i2c_master_bus_handle_t;
typedef struct i2c_master_dev_t *i2c_master_dev_handle_t;

typedef struct {
    int i2c_port;
    int sda_io_num;
    int scl_io_num;
    int clk_source;
    int glitch_ignore_cnt;
    struct {
        int enable_internal_pullup;
    } flags;
} i2c_master_bus_config_t;

typedef struct {
    int dev_addr_length;
    uint16_t device_address;
    uint32_t scl_speed_hz;
} i2c_device_config_t;

esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *cfg, i2c_master_bus_handle_t *bus);
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus, const i2c_device_config_t *cfg,
                                    i2c_master_dev_handle_t *dev);
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t dev, const uint8_t *data, size_t len, int timeout_ms);
esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t dev, const uint8_t *write_data, size_t write_len,
                                      uint8_t *read_data, size_t read_len, int timeout_ms);

#endif
