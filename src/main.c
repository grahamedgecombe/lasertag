#include <avr/interrupt.h>
#include <lasertag/uart.h>

int main(void)
{
  uart_init();
  sei();

  for (;;);
}

