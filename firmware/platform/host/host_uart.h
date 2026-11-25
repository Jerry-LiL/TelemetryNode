#ifndef TELEMETRY_HOST_UART_H
#define TELEMETRY_HOST_UART_H

#include <stdint.h>

int host_uart_open(const char *endpoint, uint32_t baud);
void host_uart_arm_tx(void);

#endif
