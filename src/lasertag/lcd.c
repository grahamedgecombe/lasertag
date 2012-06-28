#include <lasertag/lcd.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <lasertag/clock.h>
#include <lasertag/shift.h>

/* The number of rows and columns in the LCD. */
#define LCD_COLS 8
#define LCD_ROWS 2

/* RS pin values. */
#define LCD_CMD  false
#define LCD_DATA true

/* Command numbers. */
#define LCD_CMD_CLEAR_DISPLAY   0x01
#define LCD_CMD_RETURN_HOME     0x02
#define LCD_CMD_ENTRY_MODE_SET  0x04
#define LCD_CMD_DISPLAY_CONTROL 0x08
#define LCD_CMD_SHIFT           0x10
#define LCD_CMD_FUNCTION_SET    0x20
#define LCD_CMD_CGRAM_ADDR      0x40
#define LCD_CMD_DDRAM_ADDR      0x80

/* Function flags. */
#define LCD_FUNCTION_5x10DOTS 0x04 /* (otherwise 5x8 dots) */
#define LCD_FUNCTION_2LINES   0x08 /* (otherwise 1 line) */
#define LCD_FUNCTION_8BIT     0x10 /* (otherwise 4 bits) */

/* Display control flags. */
#define LCD_DISPLAY_ON     0x04
#define LCD_DISPLAY_CURSOR 0x02
#define LCD_DISPLAY_BLINK  0x01

/* Entry mode flags. */
#define LCD_MODE_LTR   0x02 /* otherwise RTL */
#define LCD_MODE_SHIFT 0x01 /* otherwise no shift */

/*
 * To conserve I/O pins on the AVR chip, the 6 pins required to control the LCD
 * are connected through a shift register, such that only 3 pins are required
 * on the AVR.
 *
 * These constants define the number of the output (Qx) on the shift register
 * for each pin on the LCD controller.
 *
 * Note that the RW pin is hard-wired to ground as only write mode is used by
 * this code.
 */
#define LCD_RS 0
#define LCD_EN 1
#define LCD_D4 2
#define LCD_D5 3
#define LCD_D6 4
#define LCD_D7 5

/* The port and pins used to control the shift register. */
static shift_t lcd_shift = {
  .ddr = &DDRC,
  .port = &PORTC,
  .data = PC3,
  .clock = PC4,
  .latch = PC5
};

/* Current cursor position and display flags. */
static uint8_t lcd_row, lcd_col;
static uint8_t lcd_flags;

static void lcd_write4bits(bool rs, uint8_t value)
{
  /* Write D4-D7 and RS pins, and raise the EN pin. */
  uint8_t pins = ((value & 0xF) << 2) | (1 << LCD_EN);
  if (rs)
    pins |= (1 << LCD_RS);
  shift_out(&lcd_shift, pins);

  /* Wait at least 450ns for the rising edge to be detected. */
  clock_usdelay(1);

  /* Lower the EN pin. */
  pins &= ~(1 << LCD_EN);
  shift_out(&lcd_shift, pins);

  /* Wait at least 450ns for the falling edge to be detected. */
  clock_usdelay(1);
}

static void lcd_write(bool rs, uint8_t value)
{
  /* Write the 8-bit command/data in two goes. */
  lcd_write4bits(rs, (value >> 4) & 0xF);
  lcd_write4bits(rs, value & 0xF);

  /* Lower all the pins. */
  shift_out(&lcd_shift, 0);

  /*
   * Wait for the command to complete. Note that no maximum is listed for the
   * clear display command, so this is guessed assuming it takes 1.52ms (as it
   * does the same thing as 'return home') as well as 37us*2 for each of the
   * maximum of 16*2 characters on the screen (as clearing the screen is the
   * same as setting the DDRAM address and then writing to DDRAM for each of
   * the characters.)
   */
  if (value & LCD_CMD_CLEAR_DISPLAY)
    clock_msdelay(4);
  else if (value & LCD_CMD_RETURN_HOME)
    clock_usdelay(1520);
  else
    clock_usdelay(37);
}

static void lcd_write_cursor(void)
{
  static uint8_t offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  lcd_write(LCD_CMD, LCD_CMD_DDRAM_ADDR | (lcd_col + offsets[lcd_row]));
}

static void lcd_write_flags(void)
{
  lcd_write(LCD_CMD, LCD_CMD_DISPLAY_CONTROL | lcd_flags);
}

void lcd_init(void)
{
  /* Setup shift register and ensure all outputs are low. */
  shift_init(&lcd_shift);
  shift_out(&lcd_shift, 0);

  /*
   * Initialize the controller in 4-bit mode, the following sequence of
   * commands and delays is from the HD44780 datasheet.
   */
  lcd_write4bits(LCD_CMD, 0x03);
  clock_usdelay(4100);

  lcd_write4bits(LCD_CMD, 0x03);
  clock_usdelay(4100);

  lcd_write4bits(LCD_CMD, 0x03);
  clock_usdelay(100);

  lcd_write4bits(LCD_CMD, 0x03);

  lcd_write4bits(LCD_CMD, 0x02);

  /* Set function flags. */
  uint8_t function = 0;
#if LCD_ROWS > 1
  function |= LCD_FUNCTION_2LINES;
#endif
  lcd_write(LCD_CMD, LCD_CMD_FUNCTION_SET | function);

  /* Set display flags. */
  lcd_write_flags();

  /* Set the entry mode. */
  lcd_write(LCD_CMD, LCD_CMD_ENTRY_MODE_SET | LCD_MODE_LTR);

  /* Clear the LCD. */
  lcd_clear();
}

void lcd_enable(void)
{
  lcd_flags |= LCD_DISPLAY_ON;
  lcd_write_flags();
}

void lcd_disable(void)
{
  lcd_flags &= ~LCD_DISPLAY_ON;
  lcd_write_flags();
}

void lcd_clear(void)
{
  lcd_row = 0;
  lcd_col = 0;
  lcd_write(LCD_CMD, LCD_CMD_CLEAR_DISPLAY);
}

void lcd_show_cursor(bool blink)
{
  lcd_flags |= LCD_DISPLAY_CURSOR;
  if (blink)
    lcd_flags |= LCD_DISPLAY_BLINK;

  lcd_write_flags();
}

void lcd_hide_cursor(void)
{
  lcd_flags &= ~(LCD_DISPLAY_CURSOR | LCD_DISPLAY_BLINK);
  lcd_write_flags();
}

void lcd_move_cursor(uint8_t col, uint8_t row)
{
  lcd_col = col;
  lcd_row = row;
  lcd_write_cursor();
}

void lcd_putc(int c)
{
  lcd_write(LCD_DATA, c);
  if (++lcd_col == LCD_COLS)
  {
    lcd_col = 0;
    if (++lcd_row == LCD_ROWS)
      lcd_row = 0;

    lcd_write_cursor();
  }
}

void lcd_puts(const char *str)
{
  char c;
  while ((c = *str++))
    lcd_putc(c);
}

void lcd_puts_p(const char *str)
{
  char c;
  while ((c = pgm_read_byte(str++)))
    lcd_putc(c);
}

void lcd_make_char(uint8_t id, const uint8_t bitmap[8])
{
  for (int off = 0; off < 8; off++)
  {
    lcd_write(LCD_CMD, LCD_CMD_CGRAM_ADDR | (id * 8 + off));
    lcd_write(LCD_DATA, bitmap[off]);
  }

  lcd_write_cursor();
}

void lcd_make_char_p(uint8_t id, const uint8_t bitmap[8])
{
  for (int off = 0; off < 8; off++)
  {
    lcd_write(LCD_CMD, LCD_CMD_CGRAM_ADDR | (id * 8 + off));
    lcd_write(LCD_DATA, pgm_read_byte(&bitmap[off]));
  }

  lcd_write_cursor();
}

