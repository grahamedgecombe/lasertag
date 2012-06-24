#include <lasertag/clock.h>
#include <avr/io.h>

void clock_init(void)
{
  /*
   * Set up Timer2 with a prescaler of 256. The waveform generator is set to
   * mode 0 - normal operation.
   *
   * As F_CPU is 16 MHz, the timer runs at 62.5 kHz - each tick lasts 16
   * microseconds. The timer overflows after 4.096 milliseconds.
   */
  TCCR2A = 0;
  TCCR2B = (1 << CS22) | (1 << CS21);
}

