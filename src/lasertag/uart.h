#ifndef LASERTAG_UART_H
#define LASERTAG_UART_H

/* Initializes the UART. */
void uart_init(void);

/*
 * Reads a single character from the UART, or returns -1 if no character is
 * available.
 */
int uart_getc(void);

/* Writes a single character to the UART. */
void uart_putc(int c);

/* Functions for writing strings from RAM and flash memory respectively. */
void uart_puts(const char *str);
void uart_puts_p(const char *str);

#endif

