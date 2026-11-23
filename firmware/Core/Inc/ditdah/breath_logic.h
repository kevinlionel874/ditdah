/* Pure breath state-machine logic — no HAL, no I2C.
 * Compiles with plain gcc so the laptop test suite (task 9) can link it.
 *
 * Call bl_feed() with every good pressure reading (tenths of hPa).
 * Call bl_feed_fail() on every mprls_poll failure.
 * bl_sensor_ok() is false once fail_streak reaches 10. */
#ifndef DITDAH_BREATH_LOGIC_H
#define DITDAH_BREATH_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { BL_BASELINE, BL_IDLE, BL_PUFF, BL_SIP } bl_state_t;

typedef struct {
    bl_state_t state;
    int32_t    baseline;           /* tenths of hPa */
    int64_t    acc;                /* fixed-point accumulator; baseline = acc/1024 */
    int32_t    puff_threshold;     /* tenths of hPa, default 100 */
    int32_t    sip_threshold;      /* tenths of hPa, default 100 */
    uint32_t   baseline_start_ms;  /* unused after BASELINE phase; kept for reference */
    uint32_t   baseline_sum;       /* sum of readings during BASELINE phase */
    uint16_t   baseline_count;     /* number of readings collected so far */
    uint8_t    fail_streak;        /* consecutive mprls_poll failures */
} bl_ctx_t;

typedef enum {
    BL_EV_NONE,
    BL_EV_PUFF_START,
    BL_EV_PUFF_END,
    BL_EV_SIP_START,
    BL_EV_SIP_END,
} bl_event_t;

void       bl_init(bl_ctx_t *ctx, int32_t puff_threshold, int32_t sip_threshold);
bl_event_t bl_feed(bl_ctx_t *ctx, int32_t deci_hpa);   /* call with each good reading */
bool       bl_sensor_ok(const bl_ctx_t *ctx);           /* false if fail_streak >= 10 */
void       bl_feed_fail(bl_ctx_t *ctx);                 /* call on each mprls_poll failure */

/* How many good readings to collect during BASELINE (1 s at 10 ms each). */
#define BL_BASELINE_COUNT 100u

#endif /* DITDAH_BREATH_LOGIC_H */
