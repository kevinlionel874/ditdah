/* morse_decoder.c — C99 Morse decoder for DitDah
 *
 * Plain C99. No dynamic memory. No global mutable state.
 * See design.md and requirements.md for the timing rules.
 */
#include "morse_decoder.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>  /* memset, strcmp */

/* -------------------------------------------------------------------------
 * Internal state layout (must fit in 64 bytes)
 * ------------------------------------------------------------------------- */
typedef struct {
    uint16_t unit;             /* current unit ms, clamped 48-200          */
    uint8_t  mode;             /* md_mode_t                                 */
    uint8_t  adaptive;         /* bool                                      */
    uint8_t  key_is_down;      /* bool                                      */
    uint8_t  letter_open;      /* bool: a press has been recorded            */
    uint8_t  word_in_progress; /* bool: a char was output since last space  */
    uint8_t  space_sent;       /* bool: space already sent for this gap     */
    uint8_t  pattern_len;      /* 0-9; 9 means >8 elements (too long)       */
    uint8_t  pattern[9];       /* 0=dit, 1=dah                              */
    uint8_t  _pad[1];
    uint32_t down_at;
    uint32_t up_at;
} md_state_t;

_Static_assert(sizeof(md_state_t) <= 64,
               "md_state_t must fit in the 64-byte opaque md_decoder_t");

/* Cast the opaque buffer to our private state type */
static md_state_t *s_of(md_decoder_t *d) {
    return (md_state_t *)(void *)d->_opaque;
}
static const md_state_t *sc_of(const md_decoder_t *d) {
    return (const md_state_t *)(const void *)d->_opaque;
}

/* -------------------------------------------------------------------------
 * Alphabet table
 * Each entry stores the pattern as a NUL-terminated string of '.' and '-'.
 * ------------------------------------------------------------------------- */
typedef struct {
    const char *pattern;
    md_kind_t   kind;
    char        ch;    /* '\0' for non-char outputs */
} md_entry_t;

static const md_entry_t ALPHABET[] = {
    /* A-Z */
    { ".-",     MD_OUT_CHAR, 'A' },
    { "-...",   MD_OUT_CHAR, 'B' },
    { "-.-.",   MD_OUT_CHAR, 'C' },
    { "-..",    MD_OUT_CHAR, 'D' },
    { ".",      MD_OUT_CHAR, 'E' },
    { "..-.",   MD_OUT_CHAR, 'F' },
    { "--.",    MD_OUT_CHAR, 'G' },
    { "....",   MD_OUT_CHAR, 'H' },
    { "..",     MD_OUT_CHAR, 'I' },
    { ".---",   MD_OUT_CHAR, 'J' },
    { "-.-",    MD_OUT_CHAR, 'K' },
    { ".-..",   MD_OUT_CHAR, 'L' },
    { "--",     MD_OUT_CHAR, 'M' },
    { "-.",     MD_OUT_CHAR, 'N' },
    { "---",    MD_OUT_CHAR, 'O' },
    { ".--.",   MD_OUT_CHAR, 'P' },
    { "--.-",   MD_OUT_CHAR, 'Q' },
    { ".-.",    MD_OUT_CHAR, 'R' },
    { "...",    MD_OUT_CHAR, 'S' },
    { "-",      MD_OUT_CHAR, 'T' },
    { "..-",    MD_OUT_CHAR, 'U' },
    { "...-",   MD_OUT_CHAR, 'V' },
    { ".--",    MD_OUT_CHAR, 'W' },
    { "-..-",   MD_OUT_CHAR, 'X' },
    { "-.--",   MD_OUT_CHAR, 'Y' },
    { "--..",   MD_OUT_CHAR, 'Z' },
    /* 0-9 */
    { "-----",  MD_OUT_CHAR, '0' },
    { ".----",  MD_OUT_CHAR, '1' },
    { "..---",  MD_OUT_CHAR, '2' },
    { "...--",  MD_OUT_CHAR, '3' },
    { "....-",  MD_OUT_CHAR, '4' },
    { ".....",  MD_OUT_CHAR, '5' },
    { "-....",  MD_OUT_CHAR, '6' },
    { "--...",  MD_OUT_CHAR, '7' },
    { "---..",  MD_OUT_CHAR, '8' },
    { "----.",  MD_OUT_CHAR, '9' },
    /* Punctuation */
    { ".-.-.-", MD_OUT_CHAR, '.' },
    { "--..--", MD_OUT_CHAR, ',' },
    { "..--..", MD_OUT_CHAR, '?' },
    { ".----.", MD_OUT_CHAR, '\'' },
    { "-..-.",  MD_OUT_CHAR, '/' },
    { "-....-", MD_OUT_CHAR, '-' },
    { ".--.-.", MD_OUT_CHAR, '@' },
    /* Special codes (checked before general unknown) */
    { "........", MD_OUT_BACKSPACE, '\0' },
    { ".-.-",    MD_OUT_ENTER,     '\0' },
    { "..--",    MD_OUT_ACCEPT,    '\0' },
};

#define ALPHABET_SIZE ((uint8_t)(sizeof(ALPHABET) / sizeof(ALPHABET[0])))

/* Convert the runtime uint8_t pattern to a '.'/'−' string and do a linear
 * search through the table.  Returns {MD_OUT_UNKNOWN, 0} on no match.    */
static md_out_t lookup(const uint8_t *pattern, uint8_t len) {
    md_out_t unknown = { MD_OUT_UNKNOWN, '\0' };

    /* 9 is the overflow sentinel (> 8 elements) */
    if (len == 0 || len == 9 || len > 8) {
        return unknown;
    }

    /* Build a small comparison string on the stack (max 8 chars + NUL) */
    char buf[9];
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = pattern[i] ? '-' : '.';
    }
    buf[len] = '\0';

    for (uint8_t i = 0; i < ALPHABET_SIZE; i++) {
        if (strcmp(ALPHABET[i].pattern, buf) == 0) {
            md_out_t o;
            o.kind = ALPHABET[i].kind;
            o.ch   = ALPHABET[i].ch;
            return o;
        }
    }
    return unknown;
}

/* -------------------------------------------------------------------------
 * Helper: update word_in_progress after emitting an output
 * ------------------------------------------------------------------------- */
static void update_wip(md_state_t *s, md_kind_t kind) {
    switch (kind) {
        case MD_OUT_CHAR:
            s->word_in_progress = 1;
            break;
        case MD_OUT_SPACE:
        case MD_OUT_ENTER:
        case MD_OUT_ACCEPT:
            s->word_in_progress = 0;
            break;
        case MD_OUT_BACKSPACE:
        case MD_OUT_UNKNOWN:
            /* unchanged */
            break;
    }
}

/* -------------------------------------------------------------------------
 * Silence check: used by md_tick and md_key_down
 *
 * Returns the number of outputs written (0–2).
 * silence_out (may be NULL) receives the computed silence duration, which
 * md_key_down uses for the adaptive-speed update before it calls this.
 * ------------------------------------------------------------------------- */
static uint8_t silence_check(md_state_t *s, uint32_t now_ms,
                              md_out_t *out, uint8_t max_out,
                              uint32_t *silence_out) {
    int32_t _sdiff = (int32_t)((uint32_t)(now_ms) - (uint32_t)(s->up_at));
    uint32_t silence = (_sdiff < 0) ? 0u : (uint32_t)_sdiff;
    if (silence > 60000u) silence = 60000u;

    if (silence_out) *silence_out = silence;

    uint8_t n = 0;

    /* End the current letter? */
    if (s->letter_open && silence >= (uint32_t)(2u * s->unit)) {
        if (n < max_out) {
            out[n] = lookup(s->pattern, s->pattern_len);
            update_wip(s, out[n].kind);
            n++;
        }
        s->pattern_len = 0;
        s->letter_open = 0;
    }

    /* Output a word-space? */
    if (!s->key_is_down && s->word_in_progress && !s->space_sent
            && silence >= (uint32_t)(5u * s->unit)) {
        if (n < max_out) {
            out[n].kind = MD_OUT_SPACE;
            out[n].ch   = '\0';
            n++;
        }
        s->word_in_progress = 0;
        s->space_sent = 1;
    }

    return n;
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

void md_init(md_decoder_t *d, uint16_t unit_ms, md_mode_t mode, bool adaptive) {
    memset(d->_opaque, 0, sizeof(d->_opaque));
    md_state_t *s = s_of(d);

    uint16_t u = unit_ms ? unit_ms : 120u;
    if (u < 48u)  u = 48u;
    if (u > 200u) u = 200u;
    s->unit     = u;
    s->mode     = (uint8_t)mode;
    s->adaptive = adaptive ? 1u : 0u;
}

uint8_t md_key_down(md_decoder_t *d, uint32_t now_ms, md_key_t key,
                    md_out_t *out, uint8_t max_out) {
    md_state_t *s = s_of(d);
    (void)key; /* Only used for guard; dual-mode dit/dah is on key_up */

    if (s->key_is_down) return 0;

    /* Silence duration (before we act) for adaptive update */
    int32_t _adiff = (int32_t)((uint32_t)(now_ms) - (uint32_t)(s->up_at));
    uint32_t silence = (_adiff < 0) ? 0u : (uint32_t)_adiff;
    if (silence > 60000u) silence = 60000u;

    /* Adaptive speed: only in-letter silences (< 2*unit) update the unit */
    if (s->adaptive && s->letter_open
            && silence < (uint32_t)(2u * s->unit)) {
        int32_t delta    = (int32_t)silence - (int32_t)s->unit;
        int32_t new_unit = (int32_t)s->unit + delta / 4;
        if (new_unit < 48)  new_unit = 48;
        if (new_unit > 200) new_unit = 200;
        s->unit = (uint16_t)new_unit;
    }

    /* Run the silence check (may close the letter and/or emit a space) */
    uint8_t n = silence_check(s, now_ms, out, max_out, NULL);

    s->down_at     = now_ms;
    s->key_is_down = 1;

    return n;
}

uint8_t md_key_up(md_decoder_t *d, uint32_t now_ms, md_key_t key,
                  md_out_t *out, uint8_t max_out) {
    md_state_t *s = s_of(d);
    (void)out;
    (void)max_out;

    if (!s->key_is_down) return 0;

    /* Classify the press */
    uint8_t is_dit;
    if (s->mode == (uint8_t)MD_DUAL) {
        is_dit = (key == MD_KEY_DIT) ? 1u : 0u;
    } else {
        int32_t _pdiff = (int32_t)((uint32_t)(now_ms) - (uint32_t)(s->down_at));
        uint32_t press = (_pdiff < 0) ? 0u : (uint32_t)_pdiff;
        if (press > 60000u) press = 60000u;
        is_dit = (press < (uint32_t)(2u * s->unit)) ? 1u : 0u;
    }

    /* Append to pattern */
    if (s->pattern_len < 8u) {
        s->pattern[s->pattern_len] = is_dit ? 0u : 1u;
        s->pattern_len++;
    } else {
        /* Mark overflow: more than 8 elements → unknown */
        s->pattern_len = 9u;
    }

    s->up_at       = now_ms;
    s->letter_open = 1;
    s->space_sent  = 0;
    s->key_is_down = 0;

    return 0;
}

uint8_t md_tick(md_decoder_t *d, uint32_t now_ms,
                md_out_t *out, uint8_t max_out) {
    md_state_t *s = s_of(d);
    if (s->key_is_down) return 0;
    return silence_check(s, now_ms, out, max_out, NULL);
}

uint8_t md_flush(md_decoder_t *d, md_out_t *out, uint8_t max_out) {
    md_state_t *s = s_of(d);

    if (!s->letter_open || max_out == 0) return 0;

    out[0] = lookup(s->pattern, s->pattern_len);
    update_wip(s, out[0].kind);
    s->pattern_len = 0;
    s->letter_open = 0;

    return 1;
}

uint16_t md_unit_ms(const md_decoder_t *d) {
    return sc_of(d)->unit;
}
