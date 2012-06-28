#ifndef LASERTAG_SPI_H
#define LASERTAG_SPI_H

#include <stdint.h>

/* Initializes the SPI bus. */
void spi_init(void);

/* Transfers a single byte over the SPI bus. */
uint8_t spi_transfer(uint8_t value);

#endif

