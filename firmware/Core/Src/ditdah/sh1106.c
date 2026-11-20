#include "ditdah/sh1106.h"

#include "ditdah/font5x7.h"
#include "i2c.h"

#define SH1106_ADDR (0x3Cu << 1)
#define SH1106_TIMEOUT_MS 10u
#define SH1106_CTRL_CMD 0x00u
#define SH1106_CTRL_DATA 0x40u
#define SH1106_RAM_COLS 132u   /* the SH1106 has 132 columns of RAM ... */
#define SH1106_COL_OFFSET 2u   /* ... and the 128 visible ones start at column 2 */
#define SH1106_WIDTH 128u
#define SH1106_CHUNK 16u       /* bytes per I2C write: about 1.5 ms at 100 kHz */

static bool write_cmds(const uint8_t *cmds, uint16_t n)
{
  return HAL_I2C_Mem_Write(&hi2c1, SH1106_ADDR, SH1106_CTRL_CMD, I2C_MEMADD_SIZE_8BIT,
                           (uint8_t *)cmds, n, SH1106_TIMEOUT_MS) == HAL_OK;
}

static bool write_data(const uint8_t *data, uint16_t n)
{
  while (n > 0u)
  {
    uint16_t len = (n > SH1106_CHUNK) ? SH1106_CHUNK : n;
    if (HAL_I2C_Mem_Write(&hi2c1, SH1106_ADDR, SH1106_CTRL_DATA, I2C_MEMADD_SIZE_8BIT,
                          (uint8_t *)data, len, SH1106_TIMEOUT_MS) != HAL_OK)
    {
      return false;
    }
    data += len;
    n -= len;
  }
  return true;
}

/* raw_col counts RAM columns (0..131), not visible pixels. */
static bool set_position(uint8_t page, uint8_t raw_col)
{
  const uint8_t cmds[3] = {
    (uint8_t)(0xB0u | (page & 0x07u)),
    (uint8_t)(0x00u | (raw_col & 0x0Fu)),
    (uint8_t)(0x10u | (raw_col >> 4)),
  };
  return write_cmds(cmds, sizeof cmds);
}

bool sh1106_init(void)
{
  static const uint8_t init_cmds[] = {
    0xAE,       /* display off */
    0xD5, 0x80, /* clock divide */
    0xA8, 0x3F, /* multiplex: 64 rows */
    0xD3, 0x00, /* display offset 0 */
    0x40,       /* start line 0 */
    0xAD, 0x8B, /* DC-DC converter on */
    0xA1,       /* columns mirrored (module is mounted flipped) */
    0xC8,       /* rows scanned bottom to top */
    0xDA, 0x12, /* COM pins layout */
    0x81, 0x80, /* contrast */
    0xD9, 0x22, /* pre-charge */
    0xDB, 0x35, /* VCOM level */
    0x32,       /* pump voltage 8 V */
    0xA4,       /* show RAM contents */
    0xA6,       /* normal, not inverted */
  };
  static const uint8_t display_on = 0xAF;

  if (!write_cmds(init_cmds, sizeof init_cmds))
  {
    return false;
  }
  if (!sh1106_clear())
  {
    return false;
  }
  return write_cmds(&display_on, 1u);
}

bool sh1106_clear(void)
{
  static const uint8_t zeros[SH1106_CHUNK] = {0};

  for (uint8_t page = 0; page < SH1106_LINES; page++)
  {
    if (!set_position(page, 0u))
    {
      return false;
    }
    for (uint8_t col = 0; col < SH1106_RAM_COLS; col += SH1106_CHUNK)
    {
      uint16_t len = (uint16_t)((SH1106_RAM_COLS - col) < SH1106_CHUNK ? (SH1106_RAM_COLS - col) : SH1106_CHUNK);
      if (!write_data(zeros, len))
      {
        return false;
      }
    }
  }
  return true;
}

bool sh1106_line(uint8_t line, const char *text)
{
  uint8_t pixels[SH1106_WIDTH] = {0};
  uint16_t x = 0;

  if (line >= SH1106_LINES)
  {
    return false;
  }
  for (uint8_t i = 0; i < SH1106_CHARS_PER_LINE && text != 0 && text[i] != '\0'; i++)
  {
    char c = text[i];
    if (c < FONT5X7_FIRST || c > FONT5X7_LAST)
    {
      c = '?';
    }
    const uint8_t *glyph = font5x7[c - FONT5X7_FIRST];
    for (uint8_t col = 0; col < FONT5X7_WIDTH; col++)
    {
      pixels[x++] = glyph[col];
    }
    x++; /* one blank column between characters */
  }
  if (!set_position(line, (uint8_t)SH1106_COL_OFFSET))
  {
    return false;
  }
  return write_data(pixels, SH1106_WIDTH);
}
