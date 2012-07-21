#include <lasertag/game.h>
#include <avr/io.h>
#include <lasertag/button.h>

static button_t button_trigger = {
  .ddr = &DDRD,
  .pin = &PIND,
  .button = PD4
};

static button_t button_reload = {
  .ddr = &DDRD,
  .pin = &PIND,
  .button = PD5
};

static button_t button_mode = {
  .ddr = &DDRD,
  .pin = &PIND,
  .button = PD7
};

void game_init(void)
{
  button_init(&button_trigger);
  button_init(&button_reload);
  button_init(&button_mode);
}

void game_cycle(void)
{
  button_cycle(&button_trigger);
  button_cycle(&button_reload);
  button_cycle(&button_mode);
}

