#ifndef LASERTAG_SHIFT_H
#define LASERTAG_SHIFT_H

#include <stdint.h>

typedef struct
{
  /*
   * Pointers to the data direction and output registers of the port the shift
   * register is connected to.
   */
  volatile uint8_t *ddr;
  volatile uint8_t *port;

  /* Numbers of the data, clock and latch pins. */
  uint8_t data, clock, latch;
} shift_t;

/* Initializes the specified shift register. */
void shift_init(shift_t *shift);

/* Writes 8 bits of data to the specified shift register. */
void shift_out(shift_t *shift, uint8_t data);

#endif

