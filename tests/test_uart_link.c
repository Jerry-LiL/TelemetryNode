#include "test_harness.h"
#include "uart_link.h"

#include <string.h>

void board_uart_arm_tx(void)
{
}

int main(void)
{
    uint8_t byte = 0;
    uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    int i;

    uart_link_init();
    EXPECT(uart_link_try_recv(&byte) != 0);
    uart_link_isr_rx(0x41);
    uart_link_isr_rx(0x42);
    EXPECT(uart_link_try_recv(&byte) == 0);
    EXPECT(byte == 0x41);
    EXPECT(uart_link_try_recv(&byte) == 0);
    EXPECT(byte == 0x42);
    EXPECT(uart_link_try_recv(&byte) != 0);

    EXPECT(uart_link_send(payload, sizeof payload) == 0);
    for (i = 0; i < 8; i++) {
        EXPECT(uart_link_isr_pop_tx(&byte) == 0);
        EXPECT(byte == payload[i]);
    }
    EXPECT(uart_link_isr_pop_tx(&byte) != 0);

    uart_link_init();
    for (i = 0; i < 2000; i++) {
        uart_link_isr_rx((uint8_t)i);
    }
    EXPECT(uart_link_rx_overflows() > 0u);

    return g_test_fails == 0 ? 0 : 1;
}
