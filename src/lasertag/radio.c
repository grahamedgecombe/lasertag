#include <lasertag/radio.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <lasertag/spi.h>

static uint16_t radio_spi_transfer(uint16_t value)
{
  /* Lower SS'. */
  PORTB &= ~(1 << PB2);

  /* Transfer most significant 8 bits. */
  uint16_t ret_value = spi_transfer((value >> 8) & 0xFF) << 8;

  /* Transfer the least significant 8 bits. */
  ret_value |= spi_transfer(value & 0xFF);

  /* Raise SS'. */
  PORTB |= (1 << PB2);

  /* Return the return value. */
  return ret_value;
}

ISR(INT1_vect)
{

}

void radio_init(void)
{
  /* Set SS' (PB2) to be an output and raise it. */
  DDRB |= (1 << PB2);
  PORTB |= (1 << PB2);

  /* Set nIRQ (PD3) to be an input. */
  DDRD &= ~(1 << PD3);

  /* Enable INT1 on a falling edge of PD3. */
  EICRA |= (1 << ISC11);
  EIMSK |= (1 << INT1);
}

