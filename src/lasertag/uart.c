#include <lasertag/uart.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stddef.h>
#include <util/atomic.h>

/* The baud rate. */
#define UART_BAUD 9600

/* The number of bytes in the RX and TX buffers. */
#define UART_BUF_SIZE 16

typedef struct
{
  volatile char buf[UART_BUF_SIZE];
  volatile size_t head, tail;
} uart_ringbuf_t;

static uart_ringbuf_t uart_rx_buf, uart_tx_buf;

static bool uart_ringbuf_empty(uart_ringbuf_t *ring)
{
  return ring->head == ring->tail;
}

static bool uart_ringbuf_full(uart_ringbuf_t *ring)
{
  return ((ring->head + 1) % UART_BUF_SIZE) == ring->tail;
}

static char uart_ringbuf_pop(uart_ringbuf_t *ring)
{
  char c = ring->buf[ring->head];
  ring->head = (ring->head + 1) % UART_BUF_SIZE;
  return c;
}

static void uart_ringbuf_push(uart_ringbuf_t *ring, char c)
{
  ring->buf[ring->tail] = c;
  ring->tail = (ring->tail + 1) % UART_BUF_SIZE;
}

void uart_init(void)
{
  /* Set PD0 (UART RX) to be an input. */
  DDRD &= ~(1 << PD0);

  /* Set PD1 (UART TX) to be an output. */
  DDRD |= (1 << PD1);

  /* Set the baud rate register. */
  uint16_t ubrr = F_CPU / (16UL * UART_BAUD) - 1;
  UBRR0H = (uint8_t) (ubrr >> 8);
  UBRR0L = (uint8_t) ubrr;

  /*
   * Reset UCSR0A - even though we don't set any flags, and the datasheet
   * states the register will be zero on boot, the boot loader may set some
   * flags in this register (e.g. the Arduino boot loader on a pre-R3 UNO board
   * sets the 2X flag in this register.)
   */
  UCSR0A = 0;

  /* Enable receiver/transmitter and unmask the RX interrupt. */
  UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);

  /*
   * Set the frame format:
   *   - async mode
   *   - 8 bit data
   *   - no parity bit
   *   - 1 start bit
   *   - 1 stop bit
   */
  UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
}

ISR(USART_RX_vect)
{
  /*
   * Read the next character and push it onto the RX buffer if it is not full.
   * If the RX buffer is full, we're going too slowly to receive any more data
   * and don't have a choice but to discard it.
   */
  char c = UDR0;
  if (!uart_ringbuf_full(&uart_rx_buf))
    uart_ringbuf_push(&uart_rx_buf, c);
}

ISR(USART_UDRE_vect)
{
  /*
   * If the TX buffer has been emptied, the UDRE interrupt is masked so this
   * code will not be called again.
   *
   * Otherwise, a character is popped from the TX buffer and written to the UDR
   * register. When this write is complete, this interrupt will fire again, in
   * order for the next character(s) to be sent.
   */
  if (uart_ringbuf_empty(&uart_tx_buf))
    UCSR0B &= ~(1 << UDRIE0);
  else
    UDR0 = uart_ringbuf_pop(&uart_tx_buf);
}

int uart_getc(void)
{
  /* Pop and return a character from the RX buffer, or -1 if it is empty. */
  int c = -1;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    if (!uart_ringbuf_empty(&uart_rx_buf))
      c = uart_ringbuf_pop(&uart_rx_buf);
  }
  return c;
}

void uart_putc(int c)
{
  /* Spin until the TX buffer has room for the character. */
  for (bool done = false; !done;)
  {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
      if (!uart_ringbuf_full(&uart_tx_buf))
      {
        /* Push the character into the buffer. */
        uart_ringbuf_push(&uart_tx_buf, c);

        /*
         * Unmask the UDRE interrupt. When the UDR register is empty, the UDRE
         * interrupt is fired and a character will be written from the buffer
         * to the UDR register.
         */
        UCSR0B |= (1 << UDRIE0);

        done = true;
      }
    }
  }
}

void uart_puts(const char *str)
{
  char c;
  while ((c = *str++))
    uart_putc(c);
}

void uart_puts_p(const char *str)
{
  char c;
  while ((c = pgm_read_byte(str++)))
    uart_putc(c);
}

