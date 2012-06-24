#ifndef LASERTAG_IR_H
#define LASERTAG_IR_H

#include <stdbool.h>
#include <stdint.h>

void ir_init(void);
void ir_tx(uint16_t packet);
bool ir_rx(uint16_t *packet);

#endif

