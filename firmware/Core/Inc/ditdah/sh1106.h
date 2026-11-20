/* Minimal SH1106 128x64 OLED driver over I2C1 (address 0x3C), text only.
 * 8 text lines (0..7) of 21 characters, using the 5x7 font.
 * Every I2C transfer has a 10 ms timeout; functions return false on any I2C error. */
#ifndef DITDAH_SH1106_H
#define DITDAH_SH1106_H

#include <stdbool.h>
#include <stdint.h>

#define SH1106_LINES 8u
#define SH1106_CHARS_PER_LINE 21u

bool sh1106_init(void);
bool sh1106_clear(void);
/* Draws a whole line, padding with spaces so old text disappears. */
bool sh1106_line(uint8_t line, const char *text);

#endif
