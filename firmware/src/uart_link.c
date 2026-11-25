#include "uart_link.h"

#include "board.h"

#include <stdatomic.h>
#include <string.h>

#define UART_RING_SIZE 1024u

typedef struct {
    uint8_t data[UART_RING_SIZE];
    atomic_uint head;
    atomic_uint tail;
} uart_ring_t;

static uart_ring_t s_tx;
static uart_ring_t s_rx;
static atomic_uint s_rx_overflow;
static atomic_uint s_tx_dropped;

static int ring_push(uart_ring_t *ring, uint8_t byte)
{
    unsigned int tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    unsigned int next = (tail + 1u) & (UART_RING_SIZE - 1u);
    unsigned int head = atomic_load_explicit(&ring->head, memory_order_acquire);

    if (next == head) {
        return -1;
    }
    ring->data[tail] = byte;
    atomic_store_explicit(&ring->tail, next, memory_order_release);
    return 0;
}

static int ring_pop(uart_ring_t *ring, uint8_t *byte)
{
    unsigned int head = atomic_load_explicit(&ring->head, memory_order_relaxed);
    unsigned int tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

    if (head == tail) {
        return -1;
    }
    *byte = ring->data[head];
    atomic_store_explicit(&ring->head, (head + 1u) & (UART_RING_SIZE - 1u), memory_order_release);
    return 0;
}

static unsigned int ring_free(const uart_ring_t *ring)
{
    unsigned int head = atomic_load_explicit(&ring->head, memory_order_acquire);
    unsigned int tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    return (UART_RING_SIZE - 1u) - ((tail - head) & (UART_RING_SIZE - 1u));
}

void uart_link_init(void)
{
    memset(s_tx.data, 0, sizeof s_tx.data);
    memset(s_rx.data, 0, sizeof s_rx.data);
    atomic_store(&s_tx.head, 0u);
    atomic_store(&s_tx.tail, 0u);
    atomic_store(&s_rx.head, 0u);
    atomic_store(&s_rx.tail, 0u);
    atomic_store(&s_rx_overflow, 0u);
    atomic_store(&s_tx_dropped, 0u);
}

int uart_link_send(const uint8_t *data, size_t len)
{
    size_t i;

    if ((data == NULL) && (len > 0u)) {
        return -1;
    }
    if (ring_free(&s_tx) < len) {
        atomic_fetch_add(&s_tx_dropped, 1u);
        return -1;
    }
    for (i = 0; i < len; i++) {
        if (ring_push(&s_tx, data[i]) != 0) {
            atomic_fetch_add(&s_tx_dropped, 1u);
            return -1;
        }
    }
    if (len > 0u) {
        board_uart_arm_tx();
    }
    return 0;
}

int uart_link_try_recv(uint8_t *byte)
{
    if (byte == NULL) {
        return -1;
    }
    return ring_pop(&s_rx, byte);
}

void uart_link_isr_rx(uint8_t byte)
{
    if (ring_push(&s_rx, byte) != 0) {
        atomic_fetch_add(&s_rx_overflow, 1u);
    }
}

int uart_link_isr_pop_tx(uint8_t *byte)
{
    if (byte == NULL) {
        return -1;
    }
    return ring_pop(&s_tx, byte);
}

uint32_t uart_link_rx_overflows(void)
{
    return atomic_load(&s_rx_overflow);
}

uint32_t uart_link_tx_dropped(void)
{
    return atomic_load(&s_tx_dropped);
}
