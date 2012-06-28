#ifndef LASERTAG_LCD_H
#define LASERTAG_LCD_H

#include <stdbool.h>
#include <stdint.h>

/* Initializes the LCD controller. */
void lcd_init(void);

/* Enable/disable the LCD display. */
void lcd_enable(void);
void lcd_disable(void);

/* Clears the LCD display. */
void lcd_clear(void);

/* Show/hide the cursor. */
void lcd_show_cursor(bool blink);
void lcd_hide_cursor(void);

/* Move the cursor. */
void lcd_move_cursor(uint8_t col, uint8_t row);

/* Writes a single character to the LCD display. */
void lcd_putc(int c);

/* Functions for writing strings from RAM and flash memory respectively. */
void lcd_puts(const char *str);
void lcd_puts_p(const char *str);

/* Functions for defining custom characters. */
void lcd_make_char(uint8_t id, const uint8_t bitmap[8]);
void lcd_make_char_p(uint8_t id, const uint8_t bitmap[8]);

#endif

