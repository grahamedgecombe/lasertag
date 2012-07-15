#include <lasertag/button.h>
#include <lasertag/clock.h>

/* The number of microseconds between samples. */
#define BUTTON_SAMPLE_DELAY 10000

/* The pattern of high/low samples required for the button to turn on/off. */
#define BUTTON_SAMPLE_MASK  0x1F

void button_init(button_t *button)
{
  /* Set pin to input. */
  *button->ddr &= ~(1 << button->button);
}

void button_cycle(button_t *button)
{
  uint32_t now = clock_micros();
  if (clock_delta(now, button->sampled_at) >= BUTTON_SAMPLE_DELAY)
  {
    /* Sample the current state of the button. */
    button->sampled_at = now;
    button->samples <<= 1;
    if (*button->pin & (1 << button->button))
      button->samples |= 0x1;

    /* Update the 'pressed' variable if the samples match the pattern. */
    if ((button->samples & BUTTON_SAMPLE_MASK) == BUTTON_SAMPLE_MASK)
      button->pressed = true;
    else if (((~button->samples) & BUTTON_SAMPLE_MASK) == BUTTON_SAMPLE_MASK)
      button->pressed = false;
  }
}

