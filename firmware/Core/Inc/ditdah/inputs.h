#ifndef DITDAH_INPUTS_H
#define DITDAH_INPUTS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    IN_BIG = 0,
    IN_JACK_DIT,
    IN_JACK_DAH,
    IN_BTN_DIT,
    IN_BTN_DAH,
    IN_MODE,
    IN_COUNT
} input_id_t;

typedef struct {
    input_id_t id;
    bool       down;
    uint32_t   at_ms;
} input_event_t;

/**
 * Read the initial pin states so no spurious events are generated at boot.
 * Call once after GPIO init.
 */
void inputs_init(uint32_t now);

/**
 * Poll all inputs. Debounces edges by 10 ms and emits one event per stable
 * transition. Returns the number of events written into `events` (0..max).
 * `at_ms` is the time of the first edge (edge-to-edge timing for the decoder).
 */
uint8_t inputs_poll(uint32_t now, input_event_t *events, uint8_t max);

#endif /* DITDAH_INPUTS_H */
