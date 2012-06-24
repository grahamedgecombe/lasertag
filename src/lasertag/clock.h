#ifndef LASERTAG_CLOCK_H
#define LASERTAG_CLOCK_H

/* The prescaler used by the general clock timer. */
#define CLOCK_PRESCALER 256

/* The number of microseconds per tick. */
#define CLOCK_USECS_PER_TICK ((CLOCK_PRESCALER * 1000000) / F_CPU)

void clock_init(void);

#endif
