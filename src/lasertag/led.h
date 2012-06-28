#ifndef LASERTAG_LED_H
#define LASERTAG_LED_H

typedef enum
{
  LED_RED = 0x1,
  LED_GREEN = 0x2,
  LED_YELLOW = 0x4,
  LED_BLUE = 0x8
} led_color_t;

/* Initialize the LEDs. */
void led_init(void);

/* Called regularly to turn LEDs on/off. */
void led_cycle(void);

/* Flashes the muzzle LED for a small amount of time. */
void led_muz_flash(void);

/* Turn the flashing team LEDs on and off respectively. */
void led_team_on(led_color_t color, led_color_t alt_color);
void led_team_off(void);

#endif

