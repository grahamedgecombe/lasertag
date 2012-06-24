#include <avr/interrupt.h>
#include <lasertag/clock.h>
#include <lasertag/ir.h>
#include <lasertag/uart.h>

int main(void)
{
  uart_init();
  clock_init();
  ir_init();
  sei();

  for (;;);
}

