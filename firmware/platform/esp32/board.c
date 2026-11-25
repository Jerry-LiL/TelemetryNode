#include "esp32_board.h"

#include "board.h"
#include "uart_link.h"

#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "driver/twai.h"
#include "driver/uart.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

#define BME280_ADDR 0x76u

static i2c_master_dev_handle_t s_bme;
static spi_device_handle_t s_imu;
static QueueHandle_t s_uart_queue;
static int s_wdt_added;

static void uart_event_task(void *arg)
{
    (void)arg;
    for (;;) {
        uart_event_t event;
        if (xQueueReceive(s_uart_queue, &event, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (event.type == UART_DATA) {
            uint8_t buf[128];
            size_t pending = event.size;
            while (pending > 0u) {
                uint32_t chunk = (pending > sizeof buf) ? (uint32_t)sizeof buf : (uint32_t)pending;
                int got = uart_read_bytes(UART_NUM_1, buf, chunk, 0);
                int i;
                if (got <= 0) {
                    break;
                }
                for (i = 0; i < got; i++) {
                    uart_link_isr_rx(buf[i]);
                }
                pending -= (size_t)got;
            }
        }
    }
}

void esp32_board_init(void)
{
    i2c_master_bus_config_t bus;
    i2c_device_config_t dev;
    i2c_master_bus_handle_t i2c;
    spi_bus_config_t spi_bus;
    spi_device_interface_config_t spi_dev;
    twai_general_config_t can;
    twai_timing_config_t timing;
    twai_filter_config_t filter;

    memset(&bus, 0, sizeof bus);
    bus.i2c_port = I2C_NUM_0;
    bus.sda_io_num = 21;
    bus.scl_io_num = 22;
    bus.clk_source = I2C_CLK_SRC_DEFAULT;
    bus.glitch_ignore_cnt = 7;
    bus.flags.enable_internal_pullup = 1;
    if (i2c_new_master_bus(&bus, &i2c) == ESP_OK) {
        memset(&dev, 0, sizeof dev);
        dev.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        dev.device_address = BME280_ADDR;
        dev.scl_speed_hz = 400000;
        (void)i2c_master_bus_add_device(i2c, &dev, &s_bme);
    }

    memset(&spi_bus, 0, sizeof spi_bus);
    spi_bus.mosi_io_num = 23;
    spi_bus.miso_io_num = 19;
    spi_bus.sclk_io_num = 18;
    spi_bus.quadwp_io_num = -1;
    spi_bus.quadhd_io_num = -1;
    spi_bus.max_transfer_sz = 32;
    if (spi_bus_initialize(SPI2_HOST, &spi_bus, SPI_DMA_CH_AUTO) == ESP_OK) {
        memset(&spi_dev, 0, sizeof spi_dev);
        spi_dev.clock_speed_hz = 8000000;
        spi_dev.mode = 0;
        spi_dev.spics_io_num = 5;
        spi_dev.queue_size = 2;
        (void)spi_bus_add_device(SPI2_HOST, &spi_dev, &s_imu);
    }

    memset(&can, 0, sizeof can);
    can.mode = TWAI_MODE_NO_ACK;
    can.tx_io = 25;
    can.rx_io = 26;
    can.clkout_io = -1;
    can.bus_off_io = -1;
    can.tx_queue_len = 8;
    can.rx_queue_len = 8;
    timing = TWAI_TIMING_CONFIG_500KBITS();
    filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    if (twai_driver_install(&can, &timing, &filter) == ESP_OK) {
        (void)twai_start();
    }
}

uint32_t board_millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

int board_i2c_write(uint8_t addr_7bit, const uint8_t *data, size_t len)
{
    if ((addr_7bit != BME280_ADDR) || (s_bme == NULL) || (data == NULL)) {
        return -1;
    }
    return i2c_master_transmit(s_bme, data, len, 20) == ESP_OK ? 0 : -1;
}

int board_i2c_write_read(uint8_t addr_7bit, const uint8_t *wdata, size_t wlen,
                         uint8_t *rdata, size_t rlen)
{
    if ((addr_7bit != BME280_ADDR) || (s_bme == NULL) || (wdata == NULL) || (rdata == NULL)) {
        return -1;
    }
    return i2c_master_transmit_receive(s_bme, wdata, wlen, rdata, rlen, 20) == ESP_OK ? 0 : -1;
}

int board_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len)
{
    spi_transaction_t trans;
    if ((s_imu == NULL) || (tx == NULL) || (rx == NULL) || (len == 0u)) {
        return -1;
    }
    memset(&trans, 0, sizeof trans);
    trans.length = len * 8u;
    trans.tx_buffer = tx;
    trans.rx_buffer = rx;
    return spi_device_transmit(s_imu, &trans) == ESP_OK ? 0 : -1;
}

int board_uart_open(const char *endpoint, uint32_t baud)
{
    uart_config_t cfg;
    (void)endpoint;

    memset(&cfg, 0, sizeof cfg);
    cfg.baud_rate = (int)baud;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk = UART_SCLK_DEFAULT;

    if (uart_driver_install(UART_NUM_1, 1024, 1024, 16, &s_uart_queue, 0) != ESP_OK) {
        return -1;
    }
    if (uart_param_config(UART_NUM_1, &cfg) != ESP_OK) {
        return -1;
    }
    if (uart_set_pin(UART_NUM_1, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
        return -1;
    }
    if (xTaskCreate(uart_event_task, "uart_evt", 2048, NULL, 5, NULL) != pdPASS) {
        return -1;
    }
    return 0;
}

void board_uart_arm_tx(void)
{
    for (;;) {
        uint8_t buf[64];
        size_t filled = 0;

        while (filled < sizeof buf) {
            if (uart_link_isr_pop_tx(&buf[filled]) != 0) {
                break;
            }
            filled++;
        }
        if (filled == 0u) {
            return;
        }
        (void)uart_write_bytes(UART_NUM_1, buf, filled);
        if (filled < sizeof buf) {
            return;
        }
    }
}

void board_iwdg_start(uint32_t timeout_ms)
{
    esp_task_wdt_config_t cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.timeout_ms = timeout_ms;
    cfg.idle_core_mask = 0;
    cfg.trigger_panic = 1;
    (void)esp_task_wdt_reconfigure(&cfg);
}

void board_iwdg_refresh(void)
{
    /* Subscribe the caller (the supervisor task) the first time it kicks. */
    if (s_wdt_added == 0) {
        if (esp_task_wdt_add(NULL) != ESP_OK) {
            return;
        }
        s_wdt_added = 1;
    }
    (void)esp_task_wdt_reset();
}

void board_on_iwdg_reset(void (*fn)(void))
{
    /* The task watchdog panics and restarts the chip. There is no in-process hook. */
    (void)fn;
}

void board_log(const char *line)
{
    if (line != NULL) {
        printf("%s\n", line);
    }
}

int board_can_hw_send(const can_frame_t *frame)
{
    twai_message_t message;
    if ((frame == NULL) || (frame->dlc > 8u)) {
        return -1;
    }
    memset(&message, 0, sizeof message);
    message.identifier = frame->id;
    message.data_length_code = frame->dlc;
    message.flags = TWAI_MSG_FLAG_NONE;
    memcpy(message.data, frame->data, frame->dlc);
    return twai_transmit(&message, pdMS_TO_TICKS(10)) == ESP_OK ? 0 : -1;
}
