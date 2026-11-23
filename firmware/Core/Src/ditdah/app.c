#include "ditdah/app.h"

#include "main.h"
#include "morse_decoder.h"
#include "ditdah/inputs.h"
#include "ditdah/keyboard.h"
#include "ditdah/log.h"

static md_decoder_t s_dec;
static md_out_t     s_out[4];
static input_event_t s_events[IN_COUNT];

static void handle_outputs(uint8_t n)
{
    for (uint8_t i = 0u; i < n; i++) {
        switch (s_out[i].kind) {
            case MD_OUT_CHAR:
                (void)keyboard_type_char(s_out[i].ch);
                log_printf("[morse] char %c", s_out[i].ch);
                break;
            case MD_OUT_SPACE:
                (void)keyboard_type_char(' ');
                log_printf("[morse] space");
                break;
            case MD_OUT_BACKSPACE:
                (void)keyboard_type_usage(0x2Au);
                log_printf("[morse] backspace");
                break;
            case MD_OUT_ENTER:
                (void)keyboard_type_usage(0x28u);
                log_printf("[morse] enter");
                break;
            case MD_OUT_ACCEPT:
                (void)keyboard_type_usage(0x68u);
                log_printf("[morse] accept");
                break;
            case MD_OUT_UNKNOWN:
                log_printf("[morse] unknown");
                break;
            default:
                break;
        }
    }
}

void app_init(void)
{
    uint32_t now = HAL_GetTick();
    inputs_init(now);
    md_init(&s_dec, 0u, MD_SINGLE, true);
    log_printf("[app] init done, one-button mode");
}

void app_loop(void)
{
    uint32_t now = HAL_GetTick();

    uint8_t ne = inputs_poll(now, s_events, (uint8_t)IN_COUNT);
    for (uint8_t i = 0u; i < ne; i++) {
        input_event_t *ev = &s_events[i];
        if (ev->id == IN_BIG) {
            uint8_t n;
            if (ev->down) {
                n = md_key_down(&s_dec, ev->at_ms, MD_KEY_SINGLE, s_out, 4u);
            } else {
                n = md_key_up(&s_dec, ev->at_ms, MD_KEY_SINGLE, s_out, 4u);
            }
            handle_outputs(n);
        }
    }

    uint8_t n = md_tick(&s_dec, now, s_out, 4u);
    handle_outputs(n);

    keyboard_poll();
}
