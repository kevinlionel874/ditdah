/* Classic 5x7 font for printable ASCII (32..126). Each glyph is 5 columns,
 * bit 0 at the top. Draw with one blank column between glyphs (6 px per char). */
#ifndef DITDAH_FONT5X7_H
#define DITDAH_FONT5X7_H

#include <stdint.h>

#define FONT5X7_FIRST 32
#define FONT5X7_LAST 126
#define FONT5X7_WIDTH 5

extern const uint8_t font5x7[FONT5X7_LAST - FONT5X7_FIRST + 1][FONT5X7_WIDTH];

#endif
