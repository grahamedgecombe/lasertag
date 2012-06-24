#include <lasertag/speaker.h>
#include <avr/io.h>

/* The Timer0 prescaler. */
#define SPEAKER_PRESCALER 1024

/*
 * The frequency at which the speaker pin is toggled. The actual frequency of
 * the tone is half of the toggle frequency.
 */
#define SPEAKER_TOGGLE_FREQ (F_CPU / SPEAKER_PRESCALER)

void speaker_init(void)
{
  /* Set PD6 (speaker) to be an output. */
  DDRD |= (1 << PD6);

  /*
   * Configure Timer0 to use the clear on terminal count waveform generator,
   * such that the frequency can be varied. A prescaler of 1024 is used which
   * allows a range of 256 frequencies in the range ~30 Hz to ~7.8 kHz.
   */
  TCCR0A = (1 << WGM01);
  TCCR0B = (1 << CS02) | (1 << CS00);
}

void speaker_tone(int hz)
{
  /* Calculate and set the terminal count value for the specified frequency. */
  OCR0A = SPEAKER_TOGGLE_FREQ / (2 * hz) - 1;

  /* Set the terminal count and connect output compare unit A to PD6. */
  TCCR0A |= (1 << COM0A0);
}

void speaker_off(void)
{
  /* Disconnect output compare unit A from PD6. */
  TCCR0A &= ~(1 << COM0A0);
}

