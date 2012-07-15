#ifndef LASERTAG_BUTTON_H
#define LASERTAG_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  /*
   * Pointers to the data direction and input registers of the port the button
   * is connected to.
   */
  volatile uint8_t *ddr;
  volatile uint8_t *pin;

  /* The pin the button is connected to. */
  uint8_t button;

  /*
   * A flag which indicates if the button is currently pressed. This is
   * debounced.
   */
  bool pressed;

  /*
   * The last 8 samples are bit-packed into this variable, with the LSB being
   * the most recent.
   */
  uint8_t samples;

  /* The time at which the most recent sample was taken. */
  uint32_t sampled_at;
} button_t;

/* Initialize the button. */
void button_init(button_t *button);

/* Called regularly to update the button's state and debounce the input. */
void button_cycle(button_t *button);

#endif

