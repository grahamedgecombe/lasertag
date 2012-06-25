#include <lasertag/clock.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

/* The number of microseconds between overflow interrupts. */
#define CLOCK_USECS_PER_OVERFLOW (256 * CLOCK_USECS_PER_TICK)

static volatile uint32_t clock_overflows = 0;

ISR(TIMER2_OVF_vect)
{
  clock_overflows++;
}

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

  /* Enable the overflow interrupt. */
  TIMSK2 = (1 << TOIE2);
}

uint32_t clock_micros(void)
{
  uint8_t ticks;
  uint32_t overflows;

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    ticks = TCNT2;
    overflows = clock_overflows;
  }

  return (uint32_t) ticks * CLOCK_USECS_PER_TICK + overflows * CLOCK_USECS_PER_OVERFLOW;
}

uint32_t clock_delta(uint32_t now, uint32_t prev)
{
  if (now >= prev)
    return now - prev;
  else
    return UINT32_MAX - prev + now;
}

