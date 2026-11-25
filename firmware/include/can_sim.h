#ifndef TELEMETRY_CAN_SIM_H
#define TELEMETRY_CAN_SIM_H

#include "can_codec.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*can_hw_send_fn)(const can_frame_t *frame);

/* hw_send may be NULL when only the software bus is under test. */
void can_sim_init(can_hw_send_fn hw_send);
void can_sim_reset(void);
void can_sim_set_filter(uint16_t id, uint16_t mask);

/* 0 when the frame is accepted onto the simulated bus. */
int can_sim_transmit(const can_frame_t *frame);

uint32_t can_sim_tx_count(void);
uint32_t can_sim_rejected(void);
uint32_t can_sim_hw_errors(void);
const can_frame_t *can_sim_last(void);

#ifdef __cplusplus
}
#endif

#endif
