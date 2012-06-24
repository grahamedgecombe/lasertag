#ifndef LASERTAG_UART_H
#define LASERTAG_UART_H

void uart_init(void);
int uart_getc(void);
void uart_putc(int c);
void uart_puts(const char *str);
void uart_puts_p(const char *str);

#endif

