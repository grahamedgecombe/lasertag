#include <lasertag/spi.h>
#include <avr/io.h>

void spi_init(void)
{
  /* Set MOSI (PB3) and SCK (PB5) to outputs. */
  DDRB |= (1 << PB3) | (1 << PB5);

  /* Set MISO (PB4) to be an input. */
  DDRB &= ~(1 << PB4);

  /* Enable the SPI bus in master mode at 1 MHz. */
  SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
}

uint8_t spi_transfer(uint8_t value)
{
  /* Start the SPI transfer - write data to the slave. */
  SPDR = value;

  /* Wait for the transfer to finish. */
  while (!(SPSR & (1 << SPIF)));

  /* Read the data from the slave. */
  value = SPDR;

  /* Return the data that was read. */
  return value;
}

