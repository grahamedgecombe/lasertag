#include <lasertag/led.h>
#include <avr/io.h>
#include <lasertag/clock.h>
#include <lasertag/shift.h>
#include <stdbool.h>
#include <stdint.h>

/* The number of microseconds the muzzle flash LED is switched on for. */
#define LED_MUZ_USECS 100000UL

/* The number of microseconds between alternate team LED flashes. */
#define LED_TEAM_USECS 500000UL

static shift_t led_shift =
{
  .ddr = &DDRC,
  .port = &PORTC,
  .data = PC0,
  .clock = PC1,
  .latch = PC2
};

/* Muzzle LED state. */
static uint32_t led_muz_start;

/* Team LEDs state. */
static uint32_t led_team_start;
static led_color_t led_team_color, led_team_alt_color;
static bool led_team_alt;

void led_init(void)
{
  /* Setup shift register and ensure all outputs are low. */
  shift_init(&led_shift);
  shift_out(&led_shift, 0);

  /* Set PB0 (muzzle LED) to be an output. */
  DDRB |= (1 << PB0);
}

void led_cycle(void)
{
  uint32_t now = clock_micros();

  /* Turn off the muzzle LED if it has been on for its duration. */
  if (led_muz_start && (clock_delta(now, led_muz_start) >= LED_MUZ_USECS))
  {
    led_muz_start = 0;
    PORTB &= ~(1 << PB0);
  }

  /* Flash alternate team LEDs. */
  if (clock_delta(now, led_team_start) >= LED_TEAM_USECS)
  {
    led_team_start = now;
    if ((led_team_alt = !led_team_alt))
      shift_out(&led_shift, (led_team_color << 4) | led_team_alt_color);
    else
      shift_out(&led_shift, led_team_color | (led_team_alt_color << 4));
  }
}

void led_muz_flash(void)
{
  /* Record the time at which the muzzle flash pin was raised. */
  led_muz_start = clock_micros();

  /*
   * As zero is used as a special value to indicate the muzzle flash LED is
   * off, and as precise timing is not important (such precise timing cannot be
   * achieved with the code anyway), if the time happens to be zero, set it to
   * one instead.
   */
  if (led_muz_start == 0)
    led_muz_start = 1;

  /* Raise the muzzle flash pin. */
  PORTB |= (1 << PB0);
}

void led_team_on(led_color_t color, led_color_t alt_color)
{
  led_team_color = color;
  led_team_alt_color = alt_color;
}

void led_team_off(void)
{
  led_team_color = 0;
  led_team_alt_color = 0;
}

