#ifndef ESP32_SHIM_SPI_MASTER_H
#define ESP32_SHIM_SPI_MASTER_H

#include <stddef.h>
#include <stdint.h>

#ifndef ESP_OK
typedef int esp_err_t;
#define ESP_OK 0
#endif
#define SPI2_HOST 1
#define SPI_DMA_CH_AUTO 3

typedef struct spi_device_t *spi_device_handle_t;

typedef struct {
    int mosi_io_num;
    int miso_io_num;
    int sclk_io_num;
    int quadwp_io_num;
    int quadhd_io_num;
    int max_transfer_sz;
} spi_bus_config_t;

typedef struct {
    int clock_speed_hz;
    int mode;
    int spics_io_num;
    int queue_size;
} spi_device_interface_config_t;

typedef struct {
    size_t length;
    const void *tx_buffer;
    void *rx_buffer;
} spi_transaction_t;

esp_err_t spi_bus_initialize(int host, const spi_bus_config_t *bus, int dma);
esp_err_t spi_bus_add_device(int host, const spi_device_interface_config_t *dev, spi_device_handle_t *handle);
esp_err_t spi_device_transmit(spi_device_handle_t handle, spi_transaction_t *trans);

#endif
