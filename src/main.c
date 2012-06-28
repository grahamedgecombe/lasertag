#include <avr/interrupt.h>
#include <lasertag/clock.h>
#include <lasertag/ir.h>
#include <lasertag/lcd.h>
#include <lasertag/led.h>
#include <lasertag/speaker.h>
#include <lasertag/uart.h>

int main(void)
{
  uart_init();
  clock_init();
  ir_init();
  led_init();
  speaker_init();
  lcd_init();
  sei();

  for (;;);
}

