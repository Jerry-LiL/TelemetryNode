#include "board.h"

#include "bme280_model.h"
#include "host_uart.h"
#include "icm42688_model.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <stdatomic.h>

static struct timespec s_t0;
static int s_time_ready;
static atomic_uint s_iwdg_last_ms;
static uint32_t s_iwdg_timeout_ms = 1500u;
static void (*s_iwdg_reset)(void);
static pthread_t s_iwdg_thread;
static int s_iwdg_started;

uint32_t board_millis(void)
{
    struct timespec now;
    uint64_t start;
    uint64_t current;

    if (s_time_ready == 0) {
        clock_gettime(CLOCK_MONOTONIC, &s_t0);
        s_time_ready = 1;
    }
    clock_gettime(CLOCK_MONOTONIC, &now);
    start = (uint64_t)s_t0.tv_sec * 1000u + (uint64_t)s_t0.tv_nsec / 1000000u;
    current = (uint64_t)now.tv_sec * 1000u + (uint64_t)now.tv_nsec / 1000000u;
    return (uint32_t)(current - start);
}

int board_i2c_write(uint8_t addr_7bit, const uint8_t *data, size_t len)
{
    return bme280_model_write(addr_7bit, data, len);
}

int board_i2c_write_read(uint8_t addr_7bit, const uint8_t *wdata, size_t wlen,
                         uint8_t *rdata, size_t rlen)
{
    return bme280_model_write_read(addr_7bit, wdata, wlen, rdata, rlen);
}

int board_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len)
{
    return icm42688_model_transfer(tx, rx, len);
}

int board_uart_open(const char *endpoint, uint32_t baud)
{
    bme280_model_init(board_millis);
    icm42688_model_init(board_millis);
    return host_uart_open(endpoint, baud);
}

void board_uart_arm_tx(void)
{
    host_uart_arm_tx();
}

void board_log(const char *line)
{
    if (line == NULL) {
        return;
    }
    (void)write(STDERR_FILENO, line, strlen(line));
    (void)write(STDERR_FILENO, "\n", 1);
}

int board_can_hw_send(const can_frame_t *frame)
{
    /* The host has no CAN controller. The simulated bus already accepted the frame. */
    (void)frame;
    return 0;
}

static void *iwdg_thread(void *arg)
{
    (void)arg;
    for (;;) {
        uint32_t last;
        uint32_t now;

        usleep(20000);
        last = atomic_load(&s_iwdg_last_ms);
        now = board_millis();
        if ((now - last) > s_iwdg_timeout_ms) {
            if (s_iwdg_reset != NULL) {
                s_iwdg_reset();
            }
            atomic_store(&s_iwdg_last_ms, board_millis());
        }
    }
    return NULL;
}

void board_iwdg_start(uint32_t timeout_ms)
{
    if (timeout_ms > 0u) {
        s_iwdg_timeout_ms = timeout_ms;
    }
    atomic_store(&s_iwdg_last_ms, board_millis());
    if (s_iwdg_started == 0) {
        if (pthread_create(&s_iwdg_thread, NULL, iwdg_thread, NULL) == 0) {
            s_iwdg_started = 1;
        }
    }
}

void board_iwdg_refresh(void)
{
    atomic_store(&s_iwdg_last_ms, board_millis());
}

void board_on_iwdg_reset(void (*fn)(void))
{
    s_iwdg_reset = fn;
}
