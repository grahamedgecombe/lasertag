#ifndef LASERTAG_CLOCK_H
#define LASERTAG_CLOCK_H

#include <stdint.h>

/* The prescaler used by the timer. */
#define CLOCK_PRESCALER 256

/* The number of microseconds per tick. */
#define CLOCK_USECS_PER_TICK ((CLOCK_PRESCALER * 1000000) / F_CPU)

/* Initializes the general-purpose 'clock' timer. */
void clock_init(void);

/* Returns the number of microseconds since the clock started. */
uint32_t clock_micros(void);

/*
 * Returns the number of microseconds between two times, taking overflow into
 * account.
 */
uint32_t clock_delta(uint32_t now, uint32_t prev);

#endif

