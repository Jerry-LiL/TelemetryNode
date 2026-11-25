#ifndef ESP32_SHIM_TWAI_H
#define ESP32_SHIM_TWAI_H

#include "freertos/FreeRTOS.h"

#include <stdint.h>

#ifndef ESP_OK
typedef int esp_err_t;
#define ESP_OK 0
#endif
#define TWAI_MODE_NO_ACK 2
#define TWAI_MSG_FLAG_NONE 0u

typedef struct {
    int mode;
    int tx_io;
    int rx_io;
    int clkout_io;
    int bus_off_io;
    int tx_queue_len;
    int rx_queue_len;
    int alerts_enabled;
    int clkout_divider;
} twai_general_config_t;

typedef struct {
    int tseg_1;
    int tseg_2;
    int sjw;
    int triple_sampling;
} twai_timing_config_t;

typedef struct {
    uint32_t acceptance_code;
    uint32_t acceptance_mask;
    int single_filter;
} twai_filter_config_t;

typedef struct {
    uint32_t flags;
    uint32_t identifier;
    uint8_t data_length_code;
    uint8_t data[8];
} twai_message_t;

static inline twai_timing_config_t twai_timing_config_500kbits(void)
{
    twai_timing_config_t timing;
    timing.tseg_1 = 15;
    timing.tseg_2 = 4;
    timing.sjw = 3;
    timing.triple_sampling = 0;
    return timing;
}

static inline twai_filter_config_t twai_filter_accept_all(void)
{
    twai_filter_config_t filter;
    filter.acceptance_code = 0;
    filter.acceptance_mask = 0xFFFFFFFFu;
    filter.single_filter = 1;
    return filter;
}

#define TWAI_TIMING_CONFIG_500KBITS() twai_timing_config_500kbits()
#define TWAI_FILTER_CONFIG_ACCEPT_ALL() twai_filter_accept_all()

esp_err_t twai_driver_install(const twai_general_config_t *general, const twai_timing_config_t *timing,
                              const twai_filter_config_t *filter);
esp_err_t twai_start(void);
esp_err_t twai_transmit(const twai_message_t *message, TickType_t wait);

#endif
