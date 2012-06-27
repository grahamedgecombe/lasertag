#include <lasertag/lcd.h>
#include <avr/io.h>
#include <lasertag/shift.h>

static shift_t lcd_shift;

void lcd_init(void)
{
  /* Setup shift register. */
  lcd_shift.ddr = &DDRC;
  lcd_shift.port = &PORTC;
  lcd_shift.data = PC3;
  lcd_shift.clock = PC4;
  lcd_shift.latch = PC5;
  shift_init(&lcd_shift);
}

