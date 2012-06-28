#include <lasertag/lcd.h>
#include <avr/io.h>
#include <lasertag/shift.h>

static shift_t lcd_shift = {
  .ddr = &DDRC,
  .port = &PORTC,
  .data = PC3,
  .clock = PC4,
  .latch = PC5
};

void lcd_init(void)
{
  /* Setup shift register. */
  shift_init(&lcd_shift);
}

