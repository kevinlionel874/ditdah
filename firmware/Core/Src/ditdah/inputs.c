#include "ditdah/inputs.h"

#include "main.h"
#include "ditdah/log.h"

#define DEBOUNCE_MS 10u

typedef struct {
    input_id_t    id;
    const char   *name;
    GPIO_TypeDef *port;
    uint16_t      pin;
    bool          raw;          /* last GPIO reading */
    bool          stable;       /* debounced state */
    uint32_t      first_edge_ms;/* timestamp of the most recent raw change */
} input_def_t;

static input_def_t s_inputs[IN_COUNT] = {
    { IN_BIG,      "BIG",  BTN_BIG_GPIO_Port,  BTN_BIG_Pin,  false, false, 0u },
    { IN_JACK_DIT, "JDIT", JACK_DIT_GPIO_Port, JACK_DIT_Pin, false, false, 0u },
    { IN_JACK_DAH, "JDAH", JACK_DAH_GPIO_Port, JACK_DAH_Pin, false, false, 0u },
    { IN_BTN_DIT,  "DIT",  BTN_DIT_GPIO_Port,  BTN_DIT_Pin,  false, false, 0u },
    { IN_BTN_DAH,  "DAH",  BTN_DAH_GPIO_Port,  BTN_DAH_Pin,  false, false, 0u },
    { IN_MODE,     "MODE", BTN_MODE_GPIO_Port,  BTN_MODE_Pin, false, false, 0u },
};

void inputs_init(uint32_t now)
{
    for (uint8_t i = 0u; i < (uint8_t)IN_COUNT; i++) {
        input_def_t *in = &s_inputs[i];
        bool down = HAL_GPIO_ReadPin(in->port, in->pin) == GPIO_PIN_RESET;
        in->raw           = down;
        in->stable        = down;
        in->first_edge_ms = now;
        if (down) {
            log_printf("[input] %s is already down at start", in->name);
        }
    }
}

uint8_t inputs_poll(uint32_t now, input_event_t *events, uint8_t max)
{
    uint8_t count = 0u;

    for (uint8_t i = 0u; i < (uint8_t)IN_COUNT && count < max; i++) {
        input_def_t *in = &s_inputs[i];
        bool down = HAL_GPIO_ReadPin(in->port, in->pin) == GPIO_PIN_RESET;

        if (down != in->raw) {
            in->raw           = down;
            in->first_edge_ms = now;
        } else if (down != in->stable && (now - in->first_edge_ms) >= DEBOUNCE_MS) {
            in->stable = down;
            log_printf("[input] %s %s", in->name, down ? "down" : "up");
            events[count].id    = in->id;
            events[count].down  = down;
            events[count].at_ms = in->first_edge_ms;
            count++;
        }
    }
    return count;
}
