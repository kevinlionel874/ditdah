/* Character -> USB HID key usage on a US keyboard layout. Pure code with no HAL,
 * so it can also be compiled and tested on a laptop. */
#ifndef DITDAH_KEYBOARD_MAP_H
#define DITDAH_KEYBOARD_MAP_H

#include <stdbool.h>
#include <stdint.h>

#define KB_MOD_LEFT_SHIFT 0x02u

#define KB_USAGE_ENTER 0x28u
#define KB_USAGE_BACKSPACE 0x2Au
#define KB_USAGE_SPACE 0x2Cu
#define KB_USAGE_F13 0x68u /* "accept the suggestion" in DitDah Studio */

/* Returns false for characters DitDah never types. Letters are always sent lowercase. */
bool keyboard_map_char(char c, uint8_t *modifier, uint8_t *usage);

#endif
