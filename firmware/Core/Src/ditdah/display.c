#include "ditdah/display.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ditdah/sh1106.h"

/* --------------------------------------------------------------------------
 * OLED layout
 *
 *   Line 0: "DitDah  10wpm  1-btn"
 *   Line 1: (unused)
 *   Line 2: "Code: .-."
 *   Line 3: "Last: a"
 *   Line 4: (unused)
 *   Line 5: last 21 chars typed
 *   Line 6: (unused)
 *   Line 7: "USB ok" or "USB not" [+ " snd off"]
 * -------------------------------------------------------------------------- */

#define REDRAW_INTERVAL_MS 30u
#define PATTERN_MAX        13u  /* max dit/dah chars in a pattern */

/* Internal representation of the 5 used display lines. */
#define NUM_USED_LINES 5u

typedef struct {
    uint8_t  line_idx;                   /* physical OLED line (0–7) */
    char     text[DISPLAY_TEXT_LEN];     /* 21 chars + NUL */
    bool     dirty;
} display_line_t;

static display_line_t s_lines[NUM_USED_LINES] = {
    { 0u, "", true  },
    { 2u, "", true  },
    { 3u, "", true  },
    { 5u, "", true  },
    { 7u, "", true  },
};

/* Indices into s_lines[] */
#define IDX_HEADER  0u
#define IDX_CODE    1u
#define IDX_LAST    2u
#define IDX_TYPED   3u
#define IDX_STATUS  4u

/* State used to build the line texts */
static char    s_mode_str[8]   = "1-btn";
static uint8_t s_wpm           = 10u;
static char    s_pattern[PATTERN_MAX + 1u] = "";
static char    s_last_char     = ' ';
static char    s_typed[DISPLAY_TEXT_LEN]   = "";
static bool    s_usb_ok            = false;
static bool    s_sound_on          = true;
static bool    s_breath_sensor_ok  = true;

static uint32_t s_last_draw_ms = 0u;
static bool     s_initialized  = false;

/* --------------------------------------------------------------------------
 * Helpers that rebuild a line text and mark it dirty
 * -------------------------------------------------------------------------- */

static void rebuild_header(void)
{
    char buf[DISPLAY_TEXT_LEN];
    (void)snprintf(buf, sizeof buf, "DitDah %2uwpm %s", (unsigned)s_wpm, s_mode_str);
    if (strncmp(s_lines[IDX_HEADER].text, buf, DISPLAY_TEXT_LEN) != 0)
    {
        (void)strncpy(s_lines[IDX_HEADER].text, buf, DISPLAY_TEXT_LEN - 1u);
        s_lines[IDX_HEADER].text[DISPLAY_TEXT_LEN - 1u] = '\0';
        s_lines[IDX_HEADER].dirty = true;
    }
}

static void rebuild_code(void)
{
    char buf[DISPLAY_TEXT_LEN];
    (void)snprintf(buf, sizeof buf, "Code: %s", s_pattern);
    if (strncmp(s_lines[IDX_CODE].text, buf, DISPLAY_TEXT_LEN) != 0)
    {
        (void)strncpy(s_lines[IDX_CODE].text, buf, DISPLAY_TEXT_LEN - 1u);
        s_lines[IDX_CODE].text[DISPLAY_TEXT_LEN - 1u] = '\0';
        s_lines[IDX_CODE].dirty = true;
    }
}

static void rebuild_last(void)
{
    char buf[DISPLAY_TEXT_LEN];
    (void)snprintf(buf, sizeof buf, "Last: %c", s_last_char);
    if (strncmp(s_lines[IDX_LAST].text, buf, DISPLAY_TEXT_LEN) != 0)
    {
        (void)strncpy(s_lines[IDX_LAST].text, buf, DISPLAY_TEXT_LEN - 1u);
        s_lines[IDX_LAST].text[DISPLAY_TEXT_LEN - 1u] = '\0';
        s_lines[IDX_LAST].dirty = true;
    }
}

static void rebuild_typed(void)
{
    if (strncmp(s_lines[IDX_TYPED].text, s_typed, DISPLAY_TEXT_LEN) != 0)
    {
        (void)strncpy(s_lines[IDX_TYPED].text, s_typed, DISPLAY_TEXT_LEN - 1u);
        s_lines[IDX_TYPED].text[DISPLAY_TEXT_LEN - 1u] = '\0';
        s_lines[IDX_TYPED].dirty = true;
    }
}

static void rebuild_status(void)
{
    char buf[DISPLAY_TEXT_LEN];

    if (!s_breath_sensor_ok)
    {
        (void)strncpy(buf, "No breath sensor", DISPLAY_TEXT_LEN - 1u);
        buf[DISPLAY_TEXT_LEN - 1u] = '\0';
    }
    else if (s_sound_on)
    {
        (void)snprintf(buf, sizeof buf, "%s", s_usb_ok ? "USB ok" : "USB not");
    }
    else
    {
        (void)snprintf(buf, sizeof buf, "%s snd off", s_usb_ok ? "USB ok" : "USB not");
    }

    if (strncmp(s_lines[IDX_STATUS].text, buf, DISPLAY_TEXT_LEN) != 0)
    {
        (void)strncpy(s_lines[IDX_STATUS].text, buf, DISPLAY_TEXT_LEN - 1u);
        s_lines[IDX_STATUS].text[DISPLAY_TEXT_LEN - 1u] = '\0';
        s_lines[IDX_STATUS].dirty = true;
    }
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void display_init(void)
{
    (void)sh1106_init();

    /* Mark all used lines dirty so they are drawn on the first passes. */
    for (uint8_t i = 0u; i < NUM_USED_LINES; i++)
    {
        s_lines[i].dirty = true;
        s_lines[i].text[0] = '\0';
    }

    s_initialized  = true;
    s_last_draw_ms = 0u;

    /* Build initial texts */
    rebuild_header();
    rebuild_code();
    rebuild_last();
    rebuild_typed();
    rebuild_status();
}

void display_set_mode(const char *mode_str)
{
    if (mode_str == NULL) { return; }
    (void)strncpy(s_mode_str, mode_str, sizeof(s_mode_str) - 1u);
    s_mode_str[sizeof(s_mode_str) - 1u] = '\0';
    rebuild_header();
}

void display_set_wpm(uint8_t wpm)
{
    s_wpm = wpm;
    rebuild_header();
}

void display_set_pattern(const char *pattern)
{
    if (pattern == NULL) { return; }
    (void)strncpy(s_pattern, pattern, PATTERN_MAX);
    s_pattern[PATTERN_MAX] = '\0';
    rebuild_code();
}

void display_set_last_char(char c)
{
    s_last_char = c;
    rebuild_last();
}

void display_set_typed(const char *text)
{
    if (text == NULL) { return; }
    (void)strncpy(s_typed, text, DISPLAY_TEXT_LEN - 1u);
    s_typed[DISPLAY_TEXT_LEN - 1u] = '\0';
    rebuild_typed();
}

void display_set_usb_ok(bool ok)
{
    s_usb_ok = ok;
    rebuild_status();
}

void display_set_sound_on(bool on)
{
    s_sound_on = on;
    rebuild_status();
}

void display_set_breath_sensor_ok(bool ok)
{
    s_breath_sensor_ok = ok;
    rebuild_status();
}

void display_poll(uint32_t now)
{
    if (!s_initialized) { return; }

    /* Rate-limit: at most one redraw every 30 ms */
    if ((int32_t)(now - s_last_draw_ms) < (int32_t)REDRAW_INTERVAL_MS)
    {
        return;
    }

    /* Find the first dirty line and redraw it */
    for (uint8_t i = 0u; i < NUM_USED_LINES; i++)
    {
        if (s_lines[i].dirty)
        {
            (void)sh1106_line(s_lines[i].line_idx, s_lines[i].text);
            s_lines[i].dirty  = false;
            s_last_draw_ms    = now;
            return;  /* only one line per pass */
        }
    }
}
