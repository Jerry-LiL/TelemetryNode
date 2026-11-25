#ifndef ESP32_SHIM_UART_H
#define ESP32_SHIM_UART_H

#include "freertos/queue.h"

#include <stddef.h>
#include <stdint.h>

#ifndef ESP_OK
typedef int esp_err_t;
#define ESP_OK 0
#endif

#define UART_NUM_1 1
#define UART_PIN_NO_CHANGE (-1)
#define UART_DATA_8_BITS 3
#define UART_PARITY_DISABLE 0
#define UART_STOP_BITS_1 1
#define UART_HW_FLOWCTRL_DISABLE 0
#define UART_SCLK_DEFAULT 0

typedef enum {
    UART_DATA = 0,
    UART_BREAK = 1,
    UART_BUFFER_FULL = 2,
    UART_FIFO_OVF = 3
} uart_event_type_t;

typedef struct {
    uart_event_type_t type;
    size_t size;
} uart_event_t;

typedef struct {
    int baud_rate;
    int data_bits;
    int parity;
    int stop_bits;
    int flow_ctrl;
    int source_clk;
} uart_config_t;

esp_err_t uart_driver_install(int port, int rx_buf, int tx_buf, int queue_size, QueueHandle_t *queue, int flags);
esp_err_t uart_param_config(int port, const uart_config_t *cfg);
esp_err_t uart_set_pin(int port, int tx, int rx, int rts, int cts);
int uart_read_bytes(int port, void *buf, uint32_t len, TickType_t wait);
int uart_write_bytes(int port, const void *src, size_t len);

#endif
