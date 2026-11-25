#ifndef TELEMETRY_BOARD_H
#define TELEMETRY_BOARD_H

#include "can_codec.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int board_i2c_write(uint8_t addr_7bit, const uint8_t *data, size_t len);
int board_i2c_write_read(uint8_t addr_7bit, const uint8_t *wdata, size_t wlen,
                         uint8_t *rdata, size_t rlen);

/* Full-duplex transfer. rx may alias a buffer the caller owns; tx is read-only. */
int board_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len);

/*
 * endpoint is a filesystem path (pty symlink) or "tcp://ADDR:PORT".
 * The host build prints the path it is actually serving.
 */
int board_uart_open(const char *endpoint, uint32_t baud);
void board_uart_arm_tx(void);

uint32_t board_millis(void);

void board_iwdg_start(uint32_t timeout_ms);
void board_iwdg_refresh(void);
void board_on_iwdg_reset(void (*fn)(void));

void board_log(const char *line);

/* Platform CAN controller. The host accepts frames in software. */
int board_can_hw_send(const can_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif
