#ifndef LASERTAG_IR_H
#define LASERTAG_IR_H

#include <stdbool.h>
#include <stdint.h>

/* Initializes the infrared transmitter and receiver. */
void ir_init(void);

/*
 * Transmits a 16-bit infrared packet. No guarantees are made of the integrity
 * of the packet, or if it will even arrive.
 */
void ir_tx(uint16_t packet);

/*
 * Polls the infrared packet receive buffer. If the receive buffer is empty,
 * false is returned. Otherwise, the next packet is written into the
 * destination specified by the pointer argument and true is returned.
 */
bool ir_rx(uint16_t *packet);

#endif

