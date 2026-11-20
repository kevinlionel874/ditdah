#include "ditdah/keyboard_map.h"

bool keyboard_map_char(char c, uint8_t *modifier, uint8_t *usage)
{
  *modifier = 0u;
  if (c >= 'A' && c <= 'Z')
  {
    c = (char)(c - 'A' + 'a');
  }
  if (c >= 'a' && c <= 'z')
  {
    *usage = (uint8_t)(0x04u + (uint8_t)(c - 'a'));
    return true;
  }
  if (c >= '1' && c <= '9')
  {
    *usage = (uint8_t)(0x1Eu + (uint8_t)(c - '1'));
    return true;
  }
  switch (c)
  {
    case '0':  *usage = 0x27u; return true;
    case ' ':  *usage = KB_USAGE_SPACE; return true;
    case '-':  *usage = 0x2Du; return true;
    case '\'': *usage = 0x34u; return true;
    case ',':  *usage = 0x36u; return true;
    case '.':  *usage = 0x37u; return true;
    case '/':  *usage = 0x38u; return true;
    case '?':  *usage = 0x38u; *modifier = KB_MOD_LEFT_SHIFT; return true;
    case '@':  *usage = 0x1Fu; *modifier = KB_MOD_LEFT_SHIFT; return true;
    default:   return false;
  }
}
