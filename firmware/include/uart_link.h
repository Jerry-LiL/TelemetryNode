#ifndef TELEMETRY_UART_LINK_H
#define TELEMETRY_UART_LINK_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void uart_link_init(void);

/* Queue a buffer for the TX FIFO. Arms the transmitter. 0 on success, -1 if it did not fit. */
int uart_link_send(const uint8_t *data, size_t len);

/* Non-blocking pop of one received byte. 0 on success, -1 if the FIFO is empty. */
int uart_link_try_recv(uint8_t *byte);

/* Called from the UART interrupt (or the host model of that interrupt). */
void uart_link_isr_rx(uint8_t byte);
int uart_link_isr_pop_tx(uint8_t *byte);

uint32_t uart_link_rx_overflows(void);
uint32_t uart_link_tx_dropped(void);

#ifdef __cplusplus
}
#endif

#endif
