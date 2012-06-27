#include <lasertag/shift.h>

void shift_init(shift_t *shift)
{
  /* Set all pins to outputs. */
  *shift->ddr |= (1 << shift->data) | (1 << shift->clock) | (1 << shift->latch);
}

void shift_out(shift_t *shift, uint8_t data)
{
  /*
   * No delays are required in this function. As F_CPU is 16 MHz, the length of
   * a single AVR clock cycle is 62.5 nanoseconds. This is longer than the
   * minimum time required for the 74HC595 chip to detect a clock edge.
   */

  for (uint8_t mask = 0x80; mask != 0; mask >>= 1)
  {
    /*
     * Raise or lower the data pin depending on the value of the current
     * bit.
     */
    if (data & mask)
      *shift->port |= (1 << shift->data);
    else
      *shift->port &= ~(1 << shift->data);

    /* Pulse the clock pin. */
    *shift->port |= (1 << shift->clock);
    *shift->port &= ~(1 << shift->clock);
  }

  /* Lower the data pin. */
  *shift->port &= ~(1 << shift->data);

  /* Pulse the latch pin. */
  *shift->port |= (1 << shift->latch);
  *shift->port &= ~(1 << shift->latch);
}

