#include "ditdah/app.h"

#include <string.h>

#include "main.h"
#include "morse_decoder.h"
#include "ditdah/inputs.h"
#include "ditdah/keyboard.h"
#include "ditdah/feedback.h"
#include "ditdah/display.h"
#include "ditdah/breath.h"
#include "ditdah/log.h"

/* --------------------------------------------------------------------------
 * Decoder state
 * -------------------------------------------------------------------------- */
static md_decoder_t  s_dec;
static md_out_t      s_out[4];
static input_event_t s_events[IN_COUNT];

/* --------------------------------------------------------------------------
 * Display / typing state
 * -------------------------------------------------------------------------- */
static char     s_pattern[16];   /* dits/dahs for the letter in progress, e.g. ".-." */
static uint8_t  s_pattern_len;
static char     s_typed[22];     /* last 21 chars typed, NUL-terminated */
static uint8_t  s_typed_len;

/* --------------------------------------------------------------------------
 * Key-down tracking
 * -------------------------------------------------------------------------- */
static bool     s_key_down;      /* any key currently held */
static uint32_t s_key_down_at;   /* at_ms of the most recent key-down event */
static md_key_t s_key_down_id;   /* which key is held (for dit/dah classification) */

/* Two-switch source bitmasks (bit 0 = IN_BTN_x, bit 1 = IN_JACK_x) */
static uint8_t  s_dit_sources;
static uint8_t  s_dah_sources;

/* MD_KEY_SINGLE means no element in progress */
static md_key_t s_tw_owner;   /* the key that owns the current element */

/* Mode button tracking */
static bool     s_mode_down;
static uint32_t s_mode_down_at;
static bool     s_mode_sound_fired;

/* Sound state */
static bool     s_sound_on;

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */
static uint8_t unit_to_wpm(const md_decoder_t *d)
{
    uint16_t u = md_unit_ms(d);
    if (u == 0u) { u = 1u; }
    return (uint8_t)(1200u / (uint32_t)u);
}

/* --------------------------------------------------------------------------
 * Mode
 * -------------------------------------------------------------------------- */

typedef enum { MODE_ONE_BTN = 0, MODE_TWO_SW = 1, MODE_BREATH = 2 } app_mode_t;
static app_mode_t s_mode = MODE_ONE_BTN;

static const char *mode_name(app_mode_t m)
{
    switch (m)
    {
        case MODE_ONE_BTN: return "1-btn";
        case MODE_TWO_SW:  return "2-sw";
        case MODE_BREATH:  return "breath";
        default:           return "1-btn";
    }
}

/* --------------------------------------------------------------------------
 * Typed / pattern helpers
 * -------------------------------------------------------------------------- */

/* Append a character to s_typed, shifting left when full (keeps last 21 chars). */
static void typed_append(char c)
{
    if (s_typed_len < 21u)
    {
        s_typed[s_typed_len++] = c;
        s_typed[s_typed_len]   = '\0';
    }
    else
    {
        /* Shift left by one, drop the oldest character */
        memmove(&s_typed[0], &s_typed[1], 20u);
        s_typed[20] = c;
        s_typed[21] = '\0';
        s_typed_len = 21u;
    }
    display_set_typed(s_typed);
}

static void typed_remove_last(void)
{
    if (s_typed_len > 0u)
    {
        s_typed_len--;
        s_typed[s_typed_len] = '\0';
        display_set_typed(s_typed);
    }
}

/* Append a dit or dah symbol to s_pattern. */
static void pattern_append(char sym)
{
    if (s_pattern_len < 15u)
    {
        s_pattern[s_pattern_len++] = sym;
        s_pattern[s_pattern_len]   = '\0';
        display_set_pattern(s_pattern);
    }
}

static void pattern_clear(void)
{
    s_pattern_len  = 0u;
    s_pattern[0]   = '\0';
    display_set_pattern("");
}

/* --------------------------------------------------------------------------
 * Output handler
 * -------------------------------------------------------------------------- */
static void handle_outputs(uint8_t n)
{
    for (uint8_t i = 0u; i < n; i++)
    {
        switch (s_out[i].kind)
        {
            case MD_OUT_CHAR:
            {
                char ch = s_out[i].ch;
                /* Ensure lowercase */
                if (ch >= 'A' && ch <= 'Z')
                {
                    ch = (char)(ch + ('a' - 'A'));
                }
                (void)keyboard_type_char(ch);
                log_printf("[morse] char %c", ch);
                typed_append(ch);
                display_set_last_char(ch);
                pattern_clear();
                break;
            }
            case MD_OUT_SPACE:
                (void)keyboard_type_char(' ');
                log_printf("[morse] space");
                typed_append(' ');
                pattern_clear();
                break;

            case MD_OUT_BACKSPACE:
                (void)keyboard_type_usage(0x2Au);
                log_printf("[morse] backspace");
                typed_remove_last();
                pattern_clear();
                break;

            case MD_OUT_ENTER:
                (void)keyboard_type_usage(0x28u);
                log_printf("[morse] enter");
                pattern_clear();
                break;

            case MD_OUT_ACCEPT:
                (void)keyboard_type_usage(0x68u);
                log_printf("[morse] accept");
                pattern_clear();
                break;

            case MD_OUT_UNKNOWN:
                log_printf("[morse] unknown");
                display_set_last_char('?');
                feedback_tone(false);
                feedback_buzz_unknown();
                pattern_clear();
                break;

            default:
                break;
        }
    }
}

/* --------------------------------------------------------------------------
 * Key event processing
 * -------------------------------------------------------------------------- */
static void handle_key_down(md_key_t key, uint32_t at_ms)
{
    s_key_down    = true;
    s_key_down_at = at_ms;
    s_key_down_id = key;
    feedback_led(true);
    feedback_tone(true);
}

static void handle_key_up(md_key_t key, uint32_t at_ms)
{
    if (!s_key_down)
    {
        return;
    }

    feedback_tone(false);

    /* Classify element as dit or dah */
    char sym;
    if (s_mode == MODE_ONE_BTN)
    {
        uint32_t duration = at_ms - s_key_down_at;
        uint32_t threshold = (uint32_t)md_unit_ms(&s_dec) * 2u;
        sym = (duration < threshold) ? '.' : '-';
    }
    else
    {
        /* In dual mode the key identity decides */
        sym = (key == MD_KEY_DIT) ? '.' : '-';
    }

    pattern_append(sym);

    if (sym == '.')
    {
        feedback_motor_pulse(40u);
    }
    else
    {
        feedback_motor_pulse(120u);
    }

    s_key_down = false;
    feedback_led(false);
}

/* --------------------------------------------------------------------------
 * app_init / app_loop
 * -------------------------------------------------------------------------- */
void app_init(void)
{
    uint32_t now = HAL_GetTick();

    feedback_init();
    display_init();

    inputs_init(now);
    md_init(&s_dec, 0u, MD_SINGLE, true);

    s_mode        = MODE_ONE_BTN;
    s_pattern_len = 0u;
    s_pattern[0]  = '\0';
    s_typed_len   = 0u;
    s_typed[0]    = '\0';
    s_key_down    = false;
    s_key_down_at = 0u;
    s_key_down_id = MD_KEY_SINGLE;

    s_dit_sources      = 0u;
    s_dah_sources      = 0u;
    s_tw_owner         = MD_KEY_SINGLE;

    s_mode_down        = false;
    s_mode_down_at     = 0u;
    s_mode_sound_fired = false;

    s_sound_on = true;

    breath_init(now, 100, 100);

    display_set_mode(mode_name(s_mode));
    display_set_wpm(unit_to_wpm(&s_dec));
    display_set_usb_ok(false);

    log_printf("[app] init done, %s mode", mode_name(s_mode));
}

void app_loop(void)
{
    uint32_t now = HAL_GetTick();

    /* Poll inputs */
    uint8_t ne = inputs_poll(now, s_events, (uint8_t)IN_COUNT);

    for (uint8_t i = 0u; i < ne; i++)
    {
        input_event_t *ev = &s_events[i];

        switch (s_mode)
        {
            case MODE_ONE_BTN:
                if (ev->id == IN_BIG)
                {
                    uint8_t n;
                    if (ev->down)
                    {
                        handle_key_down(MD_KEY_SINGLE, ev->at_ms);
                        n = md_key_down(&s_dec, ev->at_ms, MD_KEY_SINGLE, s_out, 4u);
                    }
                    else
                    {
                        handle_key_up(MD_KEY_SINGLE, ev->at_ms);
                        n = md_key_up(&s_dec, ev->at_ms, MD_KEY_SINGLE, s_out, 4u);
                    }
                    handle_outputs(n);
                }
                break;

            case MODE_TWO_SW:
            {
                /* bit 0 = IN_BTN_x, bit 1 = IN_JACK_x */
                bool is_dit_btn  = (ev->id == IN_BTN_DIT);
                bool is_dit_jack = (ev->id == IN_JACK_DIT);
                bool is_dah_btn  = (ev->id == IN_BTN_DAH);
                bool is_dah_jack = (ev->id == IN_JACK_DAH);

                if (is_dit_btn || is_dit_jack)
                {
                    uint8_t bit = is_dit_btn ? 0x01u : 0x02u;
                    if (ev->down)
                    {
                        uint8_t prev = s_dit_sources;
                        s_dit_sources |= bit;
                        if (prev == 0u && s_tw_owner == MD_KEY_SINGLE)
                        {
                            s_tw_owner = MD_KEY_DIT;
                            handle_key_down(MD_KEY_DIT, ev->at_ms);
                            uint8_t n = md_key_down(&s_dec, ev->at_ms, MD_KEY_DIT, s_out, 4u);
                            handle_outputs(n);
                        }
                    }
                    else
                    {
                        s_dit_sources &= (uint8_t)~bit;
                        if (s_dit_sources == 0u && s_tw_owner == MD_KEY_DIT)
                        {
                            handle_key_up(MD_KEY_DIT, ev->at_ms);
                            uint8_t n = md_key_up(&s_dec, ev->at_ms, MD_KEY_DIT, s_out, 4u);
                            handle_outputs(n);
                            s_tw_owner = MD_KEY_SINGLE;
                        }
                    }
                }
                else if (is_dah_btn || is_dah_jack)
                {
                    uint8_t bit = is_dah_btn ? 0x01u : 0x02u;
                    if (ev->down)
                    {
                        uint8_t prev = s_dah_sources;
                        s_dah_sources |= bit;
                        if (prev == 0u && s_tw_owner == MD_KEY_SINGLE)
                        {
                            s_tw_owner = MD_KEY_DAH;
                            handle_key_down(MD_KEY_DAH, ev->at_ms);
                            uint8_t n = md_key_down(&s_dec, ev->at_ms, MD_KEY_DAH, s_out, 4u);
                            handle_outputs(n);
                        }
                    }
                    else
                    {
                        s_dah_sources &= (uint8_t)~bit;
                        if (s_dah_sources == 0u && s_tw_owner == MD_KEY_DAH)
                        {
                            handle_key_up(MD_KEY_DAH, ev->at_ms);
                            uint8_t n = md_key_up(&s_dec, ev->at_ms, MD_KEY_DAH, s_out, 4u);
                            handle_outputs(n);
                            s_tw_owner = MD_KEY_SINGLE;
                        }
                    }
                }
                break;
            }

            case MODE_BREATH:
                /* Breath events are handled below via breath_poll;
                 * physical keys are unused in this mode. */
                break;

            default:
                break;
        }

        /* IN_MODE is handled in every mode */
        if (ev->id == IN_MODE)
        {
            if (ev->down)
            {
                s_mode_down        = true;
                s_mode_down_at     = ev->at_ms;
                s_mode_sound_fired = false;
            }
            else  /* release */
            {
                if (s_mode_down && !s_mode_sound_fired)
                {
                    /* Short press (< 1 s): cycle mode */
                    uint32_t held = now - s_mode_down_at;
                    if (held < 1000u)
                    {
                        s_mode = (app_mode_t)((uint8_t)(s_mode + 1u) % 3u);

                        /* Cancel any in-progress key press cleanly */
                        if (s_key_down)
                        {
                            feedback_tone(false);
                            feedback_led(false);
                            s_key_down = false;
                        }
                        pattern_clear();
                        s_dit_sources = 0u;
                        s_dah_sources = 0u;
                        s_tw_owner    = MD_KEY_SINGLE;

                        /* Flush decoder and re-init */
                        uint8_t nf = md_flush(&s_dec, s_out, 4u);
                        handle_outputs(nf);
                        md_mode_t dec_mode = (s_mode == MODE_ONE_BTN) ? MD_SINGLE : MD_DUAL;
                        md_init(&s_dec, 0u, dec_mode, true);

                        /* Re-init breath sensor when entering breath mode. */
                        if (s_mode == MODE_BREATH)
                        {
                            breath_init(now, 100, 100);
                        }

                        /* Update display */
                        display_set_mode(mode_name(s_mode));
                        display_set_wpm(unit_to_wpm(&s_dec));
                        log_printf("[app] mode -> %s", mode_name(s_mode));
                    }
                }
                s_mode_down = false;
            }
        }
    }

    /* Check if the mode button has been held for 2 s */
    if (s_mode_down && !s_mode_sound_fired)
    {
        if ((now - s_mode_down_at) >= 2000u)
        {
            /* Toggle sound */
            s_sound_on = !s_sound_on;
            feedback_set_sound(s_sound_on);
            display_set_sound_on(s_sound_on);
            log_printf("[app] sound %s", s_sound_on ? "on" : "off");
            s_mode_sound_fired = true;
        }
    }

    /* Tick the decoder (subtract 10 ms to account for debounce) */
    {
        uint32_t tick_time = (now >= 10u) ? (now - 10u) : 0u;
        uint8_t n = md_tick(&s_dec, tick_time, s_out, 4u);
        handle_outputs(n);
    }

    display_set_wpm(unit_to_wpm(&s_dec));

    /* USB status */
    display_set_usb_ok(keyboard_connected());

    /* Breath mode: poll sensor and dispatch events */
    if (s_mode == MODE_BREATH)
    {
        uint32_t breath_time = (now >= 10u) ? (now - 10u) : 0u;
        breath_event_t bev = breath_poll(now);
        switch (bev)
        {
            case BREATH_EV_PUFF_START:
                handle_key_down(MD_KEY_DAH, breath_time);
                handle_outputs(md_key_down(&s_dec, breath_time, MD_KEY_DAH, s_out, 4u));
                break;
            case BREATH_EV_PUFF_END:
                handle_key_up(MD_KEY_DAH, breath_time);
                handle_outputs(md_key_up(&s_dec, breath_time, MD_KEY_DAH, s_out, 4u));
                break;
            case BREATH_EV_SIP_START:
                handle_key_down(MD_KEY_DIT, breath_time);
                handle_outputs(md_key_down(&s_dec, breath_time, MD_KEY_DIT, s_out, 4u));
                break;
            case BREATH_EV_SIP_END:
                handle_key_up(MD_KEY_DIT, breath_time);
                handle_outputs(md_key_up(&s_dec, breath_time, MD_KEY_DIT, s_out, 4u));
                break;
            default:
                break;
        }

        /* Update status line with sensor health */
        display_set_breath_sensor_ok(breath_ok());
    }
    else
    {
        /* Outside breath mode, always show sensor as ok on the status line. */
        display_set_breath_sensor_ok(true);
    }

    keyboard_poll();
    feedback_poll(now);
    display_poll(now);
}
