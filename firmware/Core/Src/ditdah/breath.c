/* HAL wrapper: drives the MPRLS on a 10 ms schedule and feeds the
 * breath_logic state machine. */
#include "ditdah/breath.h"

#include "ditdah/mprls.h"
#include "ditdah/log.h"

#define BREATH_POLL_INTERVAL_MS 10u
#define BREATH_LOG_INTERVAL_MS  1000u

static bl_ctx_t  s_ctx;
static uint32_t  s_last_start_ms;
static bool      s_measuring;
static bool      s_was_ok;          /* sensor-ok state from the previous poll */
static uint32_t  s_last_log_ms;     /* for the 1-s IDLE pressure log */

void breath_init(uint32_t now_ms, int32_t puff_threshold, int32_t sip_threshold)
{
    bl_init(&s_ctx, puff_threshold, sip_threshold);

    s_last_start_ms = now_ms;
    s_measuring     = false;
    s_last_log_ms   = now_ms;

    if (mprls_present())
    {
        s_was_ok = true;
        log_printf("[breath] sensor ok");
    }
    else
    {
        /* Mark sensor as already-failed so breath_ok() returns false. */
        s_ctx.fail_streak = 10u;
        s_was_ok          = false;
        log_printf("[breath] sensor not found");
    }
}

breath_event_t breath_poll(uint32_t now_ms)
{
    breath_event_t result = BREATH_EV_NONE;

    /* Start a new measurement every 10 ms. */
    if (!s_measuring && (now_ms - s_last_start_ms) >= BREATH_POLL_INTERVAL_MS)
    {
        s_measuring     = mprls_start(now_ms);
        s_last_start_ms = now_ms;
        if (!s_measuring)
        {
            bl_feed_fail(&s_ctx);
            if (!bl_sensor_ok(&s_ctx) && s_was_ok)
            {
                s_was_ok = false;
                log_printf("[breath] sensor lost");
            }
        }
    }

    if (!s_measuring)
    {
        return BREATH_EV_NONE;
    }

    int32_t deci_hpa = 0;
    mprls_status_t status = mprls_poll(now_ms, &deci_hpa);

    switch (status)
    {
        case MPRLS_PENDING:
            /* Not finished yet — come back next loop. */
            break;

        case MPRLS_READY:
        {
            s_measuring = false;
            bl_event_t ev = bl_feed(&s_ctx, deci_hpa);

            /* 1-s periodic log while in IDLE state. */
            if (s_ctx.state == BL_IDLE &&
                (now_ms - s_last_log_ms) >= BREATH_LOG_INTERVAL_MS)
            {
                log_printf("[breath] p=%ld b=%ld",
                           (long)deci_hpa,
                           (long)s_ctx.baseline);
                s_last_log_ms = now_ms;
            }

            /* Restore was_ok on a good reading. */
            s_was_ok = true;

            switch (ev)
            {
                case BL_EV_PUFF_START: result = BREATH_EV_PUFF_START; break;
                case BL_EV_PUFF_END:   result = BREATH_EV_PUFF_END;   break;
                case BL_EV_SIP_START:  result = BREATH_EV_SIP_START;  break;
                case BL_EV_SIP_END:    result = BREATH_EV_SIP_END;    break;
                default:               result = BREATH_EV_NONE;        break;
            }
            break;
        }

        case MPRLS_FAILED:
        {
            s_measuring = false;
            bl_feed_fail(&s_ctx);

            /* Log once when the sensor transitions from ok to not-ok. */
            if (s_was_ok && !bl_sensor_ok(&s_ctx))
            {
                s_was_ok = false;
                log_printf("[breath] sensor lost");
            }
            break;
        }

        default:
            s_measuring = false;
            break;
    }

    return result;
}

bool breath_ok(void)
{
    return bl_sensor_ok(&s_ctx);
}
