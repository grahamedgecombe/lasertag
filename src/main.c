#include <avr/interrupt.h>
#include <avr/io.h>
#include <lasertag/clock.h>
#include <lasertag/ir.h>
#include <lasertag/lcd.h>
#include <lasertag/led.h>
#include <lasertag/speaker.h>
#include <lasertag/spi.h>
#include <lasertag/uart.h>

int main(void)
{
  /* Power down TWI and ADC. */
  PRR = (1 << PRTWI) | (1 << PRADC);

  uart_init();
  clock_init();
  ir_init();
  led_init();
  speaker_init();
  lcd_init();
  spi_init();

  /* Enable interrupts. */
  sei();

  for (;;)
  {
    led_cycle();
  }
}

