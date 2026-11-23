/* HAL wrapper around the MPRLS sensor and breath_logic state machine.
 * Call breath_init() once at startup (or when switching into breath mode).
 * Call breath_poll() every main-loop iteration; it returns a breath_event_t
 * when a sip or puff starts or ends. */
#ifndef DITDAH_BREATH_H
#define DITDAH_BREATH_H

#include <stdbool.h>
#include <stdint.h>
#include "ditdah/breath_logic.h"

typedef enum {
    BREATH_EV_NONE,
    BREATH_EV_PUFF_START,
    BREATH_EV_PUFF_END,
    BREATH_EV_SIP_START,
    BREATH_EV_SIP_END,
} breath_event_t;

void           breath_init(uint32_t now_ms, int32_t puff_threshold, int32_t sip_threshold);
breath_event_t breath_poll(uint32_t now_ms);  /* call every loop iteration */
bool           breath_ok(void);               /* false when sensor has failed 10x in a row */

#endif /* DITDAH_BREATH_H */
