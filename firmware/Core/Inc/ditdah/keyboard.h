/* USB keyboard output. Characters are queued (32 deep) and sent one at a time as a
 * key-press report followed by an all-released report. Call keyboard_poll() every
 * main-loop pass. Needs the HID class patched by firmware/tools/patch-hid-keyboard.ps1. */
#ifndef DITDAH_KEYBOARD_H
#define DITDAH_KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "ditdah/keyboard_map.h"

bool keyboard_type_char(char c);          /* false if it can't be typed, the queue is full, or USB isn't connected */
bool keyboard_type_usage(uint8_t usage);  /* Enter, Backspace, F13 and so on */
void keyboard_poll(void);
bool keyboard_connected(void);

#endif
