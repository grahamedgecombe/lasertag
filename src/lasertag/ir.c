#include <lasertag/ir.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <lasertag/clock.h>
#include <stddef.h>
#include <util/atomic.h>

/* The frequency of the infrared carrier. */
#define IR_FREQ 38000

/*
 * The reciprocal of the duty cycle of the carrier, i.e. a value of 4 sets the
 * duty cycle to 1/4 or 25%.
 */
#define IR_DUTY_RECIPROCAL 4

/*
 * The length of header, one/zero marks and spaces in clock ticks. The last
 * value is the acceptable error on either side of the ideal value.
 */
#define IR_HEADER    (1200 / CLOCK_USECS_PER_TICK)
#define IR_MARK_ONE  (800  / CLOCK_USECS_PER_TICK)
#define IR_MARK_ZERO (400  / CLOCK_USECS_PER_TICK)
#define IR_SPACE     (400  / CLOCK_USECS_PER_TICK)
#define IR_ERROR     (200  / CLOCK_USECS_PER_TICK)

/*
 * The RX timeout in clock ticks, this is slightly longer than the maximum
 * number of ticks that the carrier is expected to be turned on for.
 */
#define IR_TIMEOUT (IR_HEADER + IR_ERROR * 2)

/*
 * The number of packets in the RX and TX buffers. As the IR receiver can only
 * manage around 800 bursts per second, the buffers can be kept small.
 */
#define IR_BUF_SIZE 4

/*
 * The possible states for the RX and TX state machines.
 *
 *  IDLE: doing nothing
 *  MARK: transmitting or receiving a mark
 * SPACE: transmitting or receiving a space
 */
typedef enum
{
  IR_STATE_IDLE,
  IR_STATE_MARK,
  IR_STATE_SPACE
} ir_state_t;

typedef struct
{
  volatile uint16_t buf[IR_BUF_SIZE];
  volatile size_t head, tail;
} ir_ringbuf_t;

/* The TX state. */
static volatile ir_state_t ir_tx_state;
static volatile uint16_t ir_tx_packet;
static uint8_t ir_tx_bit;

/* The RX state. */
static volatile ir_state_t ir_rx_state;
static volatile uint16_t ir_rx_packet;
static uint8_t ir_rx_bit, ir_rx_clock;

/* The TX and RX buffers. */
static ir_ringbuf_t ir_rx_buf, ir_tx_buf;

static bool ir_ringbuf_empty(ir_ringbuf_t *ring)
{
  return ring->head == ring->tail;
}

static bool ir_ringbuf_full(ir_ringbuf_t *ring)
{
  return ((ring->head + 1) % IR_BUF_SIZE) == ring->tail;
}

static uint16_t ir_ringbuf_pop(ir_ringbuf_t *ring)
{
  uint16_t packet = ring->buf[ring->head];
  ring->head = (ring->head + 1) % IR_BUF_SIZE;
  return packet;
}

static void ir_ringbuf_push(ir_ringbuf_t *ring, uint16_t packet)
{
  ring->buf[ring->tail] = packet;
  ring->tail = (ring->tail + 1) % IR_BUF_SIZE;
}

/*
 * The following functions turn the infrared carrier on and off by setting the
 * Timer1 channel A output compare mode to 2 and 0 respectively.
 */
static void ir_carrier_on(void)
{
  TCCR1A |= (1 << COM1A1);
}

static void ir_carrier_off(void)
{
  TCCR1A &= ~(1 << COM1A1);
}

/*
 * Schedules an RX timeout interrupt.
 *
 * NB: interrupts must be disabled by the caller.
 */
static void ir_schedule_timeout_intr(void)
{
  /* Disable the output compare interrupt. */
  TIMSK2 &= ~(1 << OCIE2B);

  /* Reset the output compare flag to mask an immediate interrupt. */
  TIFR2 |= (1 << OCF2B);

  /* Set how many clcok ticks in the future the interrupt should be fired. */
  OCR2B = TCNT2 + IR_TIMEOUT;

  /* Enable the output compare interrupt. */
  TIMSK2 |= (1 << OCIE2B);
}

/*
 * Mask any further RX timeout interrupts.
 *
 * NB: interrupts must be disabled by the caller.
 */
static void ir_mask_timeout_intr(void)
{
  TIMSK2 &= ~(1 << OCIE2B);
}

/*
 * Schedules a transmit interrupt, which is used to change the state of the
 * carrier at the end of a mark or space, or to end the transmission.
 *
 * NB: interrupts must be disabled by the caller.
 */
static void ir_schedule_tx_intr(uint8_t clock_ticks)
{
  /* Disable the output compare interrupt. */
  TIMSK2 &= ~(1 << OCIE2A);

  /* Reset the output compare flag to mask an immediate interrupt. */
  TIFR2 |= (1 << OCF2A);

  /* Set how many clcok ticks in the future the interrupt should be fired. */
  OCR2A = TCNT2 + clock_ticks;

  /* Enable the output compare interrupt. */
  TIMSK2 |= (1 << OCIE2A);
}

/*
 * Mask any further transmit interrupts.
 *
 * NB: interrupts must be disabled by the caller.
 */
static void ir_mask_tx_intr(void)
{
  TIMSK2 &= ~(1 << OCIE2A);
}

/* Calculate the delta between clock ticks taking rollover into account. */
static uint8_t ir_delta(uint8_t now, uint8_t prev)
{
  if (now >= prev)
    return now - prev;
  else
    return UINT8_MAX - prev + now;
}

/*
 * Start the transmission of a new packet - this is used in both ir_tx() and
 * the Timer2 ISR.
 *
 * NB: interrupts must be disabled by the caller.
 */
static void ir_start_tx(uint16_t packet)
{
  ir_tx_state = IR_STATE_MARK;
  ir_tx_packet = packet;
  ir_tx_bit = 16;

  ir_carrier_on();
  ir_schedule_tx_intr(IR_HEADER);
}

ISR(TIMER2_COMPB_vect)
{
  /* The receive operation has timed out, reset the RX state. */
  ir_rx_state = IR_STATE_IDLE;
  ir_mask_timeout_intr();
}

ISR(TIMER2_COMPA_vect)
{
  if (ir_tx_state == IR_STATE_MARK)
  {
    /*
     * This is the end of a mark period. Switch the carrier off and schedule
     * the next interrupt to be triggered at the end of the space period.
     */
    ir_tx_state = IR_STATE_SPACE;
    ir_carrier_off();
    ir_schedule_tx_intr(IR_SPACE);
  }
  else
  {
    /* Check if the packet has been completely transmitted. */
    if (ir_tx_bit-- == 0)
    {
      if (ir_ringbuf_empty(&ir_tx_buf))
      {
        /*
         * The transmit buffer is empty, mask any further transmit interrupts
         * and switch back to the idle state.
         */
        ir_tx_state = IR_STATE_IDLE;
        ir_mask_tx_intr();
      }
      else
      {
        /* Start transmission of the next packet in the transmit buffer. */
        ir_start_tx(ir_ringbuf_pop(&ir_tx_buf));
      }
      return;
    }

    /* Read the bit being transmitted. */
    bool bit = (ir_tx_packet & (1 << ir_tx_bit));

    /*
     * Switch back to the mark state, varying the length of the mark depending
     * on what the bit was, and re-enable the carrier.
     */
    ir_tx_state = IR_STATE_MARK;
    ir_carrier_on();
    ir_schedule_tx_intr(bit ? IR_MARK_ONE : IR_MARK_ZERO);
  }
}

ISR(INT0_vect)
{
  /* Record the current time. */
  uint8_t now = TCNT2;

  /* Read the PD2 pin, note that the TSOP is active low. */
  bool rising = !(PIND & (1 << PD2));

  if (ir_rx_state == IR_STATE_IDLE && rising)
  {
    /*
     * This means the first rising edge of a new packet was detected - i.e. the
     * start of the header mark.
     */
    ir_rx_clock = now;
    ir_rx_packet = 0;
    ir_rx_bit = 16;
    ir_rx_state = IR_STATE_MARK;
  }
  else if (ir_rx_state == IR_STATE_SPACE && rising)
  {
    /*
     * This means the rising edge of an existing packet was detected - i.e. the
     * end of a space/start of the a mark.
     */
    uint8_t delta = ir_delta(now, ir_rx_clock);

    if (delta >= (IR_SPACE - IR_ERROR) && delta <= (IR_SPACE + IR_ERROR))
    {
      /* Time the next mark. */
      ir_rx_clock = now;
      ir_rx_state = IR_STATE_MARK;
    }
    else
    {
      /* Corrupted packet - drop it. */
      ir_rx_state = IR_STATE_IDLE;
      ir_mask_timeout_intr();
      return;
    }
  }
  else if (ir_rx_state == IR_STATE_MARK && !rising)
  {
    /*
     * This means a falling edge was detected - i.e. this is the end of a mark.
     */
    uint8_t delta = ir_delta(now, ir_rx_clock);

    if (ir_rx_bit == 16)
    {
      /* This means we are looking for the header mark. */
      if (delta >= (IR_HEADER - IR_ERROR) && delta <= (IR_HEADER + IR_ERROR))
      {
        /* Time the next space. */
        ir_rx_clock = now;
        ir_rx_state = IR_STATE_SPACE;
        ir_rx_bit--;
      }
      else
      {
        /* Corrupted packet - drop it. */
        ir_rx_state = IR_STATE_IDLE;
        ir_mask_timeout_intr();
        return;
      }
    }
    else
    {
      /* This means we are looking for a zero or one mark. */
      if (delta >= (IR_MARK_ZERO - IR_ERROR) && delta <= (IR_MARK_ZERO + IR_ERROR))
      {
        /*
         * As the packet buffer is initialized to zero we don't need to set a
         * bit here.
         */
      }
      else if (delta >= (IR_MARK_ONE - IR_ERROR) && delta <= (IR_MARK_ONE + IR_ERROR))
      {
        /* Set the bit in the RX packet buffer. */
        ir_rx_packet |= (1 << ir_rx_bit);
      }
      else
      {
        /* Corrupted packet - drop it. */
        ir_rx_state = IR_STATE_IDLE;
        ir_mask_timeout_intr();
        return;
      }

      /* Check if a whole packet has now been received. */
      if (ir_rx_bit == 0)
      {
        /*
         * Push this packet onto the RX buffer, and switch back to the idle
         * state so that the next packet can be received.
         *
         * If the RX buffer is full, all we can do is drop the packet.
         */
        if (!ir_ringbuf_full(&ir_rx_buf))
          ir_ringbuf_push(&ir_rx_buf, ir_rx_packet);

        ir_rx_state = IR_STATE_IDLE;
        ir_mask_timeout_intr();
        return;
      }
      else
      {
        /* Time the next space. */
        ir_rx_clock = now;
        ir_rx_state = IR_STATE_SPACE;
        ir_rx_bit--;
      }
    }
  }
  else
  {
    /*
     * This means an interrupt happened when it was not expected (e.g. another
     * interrupt was missed or we're processing things too slowly to keep up.)
     *
     * There isn't much that can be done aside from dropping the current
     * packet.
     */
    ir_rx_state = IR_STATE_IDLE;
    ir_mask_timeout_intr();
    return;
  }

  ir_schedule_timeout_intr();
}

void ir_init(void)
{
  /* Set PB1 (IR LED) to be an output. */
  DDRB |= (1 << PB1);

  /* Set PD2 (IR receiver) to be an input. */
  DDRD &= ~(1 << PD2);

  /* Enable external interrupt INT0 on a rising or falling edge of PD2. */
  EICRA |= (1 << ISC00);
  EIMSK |= (1 << INT0);

  /*
   * Configure Timer1 to output a pulse-width modulated signal with the
   * frequency and duty cycle defined in IR_FREQ and IR_DUTY_RECIPROCAL on PB1.
   *
   * The waveform generator is set to mode 14 - fast PWM with the TOP value
   * stored in the ICR1 register. When the carrier frequency should be
   * generated, the channel A compare output mode is set to 2, which is the
   * non-inverting mode.
   *
   * TCNT1 counts from zero to ICR1. When it reaches ICR1, it is reset back to
   * zero.
   *
   * When TCNT1 is zero, PB1 is switched on. When TCNT1 is OCR1A, PB1 is
   * switched off.
   *
   * By choosing the appropriate values for ICR1 and OCR1A, the frequency and
   * duty cycle of the signal on PB1 can be controlled - ICR1 determines the
   * frequency of the signal, and adjusting OCR1A to some fraction of ICR1 sets
   * a duty cycle of said fraction.
   *
   * The clock source is set to the internal oscillator without any scaling,
   * this means it runs at F_CPU.
   */
  TCCR1A = (1 << WGM11);
  TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS10);
  TCCR1C = 0;

  uint16_t ticks = F_CPU / IR_FREQ - 1;
  ICR1 = ticks;
  OCR1A = ticks / IR_DUTY_RECIPROCAL;
}

void ir_tx(uint16_t packet)
{
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    if (ir_tx_state == IR_STATE_IDLE)
    {
      /*
       * If the transmitter is idle, we can bypass the buffer completely and
       * start transmitting the packet.
       */
      ir_start_tx(packet);
    }
    else
    {
      /*
       * Push the packet onto the transmit buffer. If the buffer is full, the
       * packet is dropped.
       */
      if (!ir_ringbuf_full(&ir_tx_buf))
        ir_ringbuf_push(&ir_tx_buf, packet);
    }
  }
}

bool ir_rx(uint16_t *packet)
{
  bool success = false;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    if (!ir_ringbuf_empty(&ir_rx_buf))
    {
      /* Pop a packet from the receive buffer. */
      *packet = ir_ringbuf_pop(&ir_rx_buf);
      success = true;
    }
  }
  return success;
}

