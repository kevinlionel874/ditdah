#ifndef DITDAH_DISPLAY_H
#define DITDAH_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#define DISPLAY_TEXT_LEN 22u   /* 21 chars + NUL */

void display_init(void);
void display_set_mode(const char *mode_str);    /* "1-btn", "2-sw", "breath" */
void display_set_wpm(uint8_t wpm);
void display_set_pattern(const char *pattern);  /* e.g. ".-." — dits/dahs in progress */
void display_set_last_char(char c);             /* last decoded letter, or '?' */
void display_set_typed(const char *text);       /* last 21 chars typed */
void display_set_usb_ok(bool ok);
void display_set_sound_on(bool on);
void display_set_breath_sensor_ok(bool ok);  /* shows "No breath sensor" on status line when false */
void display_poll(uint32_t now);  /* redraws at most one dirty line, at most every 30 ms */

#endif /* DITDAH_DISPLAY_H */
