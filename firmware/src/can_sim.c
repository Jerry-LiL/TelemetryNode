#include "can_sim.h"

#include <string.h>

static can_hw_send_fn s_hw;
static uint32_t s_tx;
static uint32_t s_rejected;
static uint32_t s_hw_errors;
static can_frame_t s_last;
static int s_filter;
static uint16_t s_filter_id;
static uint16_t s_filter_mask;

void can_sim_init(can_hw_send_fn hw_send)
{
    can_sim_reset();
    s_hw = hw_send;
}

void can_sim_reset(void)
{
    s_hw = NULL;
    s_tx = 0;
    s_rejected = 0;
    s_hw_errors = 0;
    s_filter = 0;
    s_filter_id = 0;
    s_filter_mask = 0;
    memset(&s_last, 0, sizeof s_last);
}

void can_sim_set_filter(uint16_t id, uint16_t mask)
{
    s_filter = 1;
    s_filter_id = id;
    s_filter_mask = mask;
}

int can_sim_transmit(const can_frame_t *frame)
{
    if ((frame == NULL) || (frame->dlc > 8u) || (frame->id > 0x7FFu)) {
        s_rejected++;
        return -1;
    }
    if ((s_filter != 0) && (((frame->id ^ s_filter_id) & s_filter_mask) != 0u)) {
        s_rejected++;
        return -1;
    }

    s_last = *frame;
    s_tx++;
    if ((s_hw != NULL) && (s_hw(frame) != 0)) {
        s_hw_errors++;
        return -1;
    }
    return 0;
}

uint32_t can_sim_tx_count(void)
{
    return s_tx;
}

uint32_t can_sim_rejected(void)
{
    return s_rejected;
}

uint32_t can_sim_hw_errors(void)
{
    return s_hw_errors;
}

const can_frame_t *can_sim_last(void)
{
    return &s_last;
}
