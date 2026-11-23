/* Pure breath state-machine logic.
 * No HAL headers, no logging — this file compiles with plain gcc. */
#include "ditdah/breath_logic.h"

#include <stddef.h>

void bl_init(bl_ctx_t *ctx, int32_t puff_threshold, int32_t sip_threshold)
{
    ctx->state              = BL_BASELINE;
    ctx->baseline           = 0;
    ctx->acc                = 0;
    ctx->puff_threshold     = puff_threshold;
    ctx->sip_threshold      = sip_threshold;
    ctx->baseline_start_ms  = 0u;
    ctx->baseline_sum       = 0u;
    ctx->baseline_count     = 0u;
    ctx->fail_streak        = 0u;
}

bl_event_t bl_feed(bl_ctx_t *ctx, int32_t deci_hpa)
{
    ctx->fail_streak = 0u;

    switch (ctx->state)
    {
        case BL_BASELINE:
        {
            /* Accumulate readings for BL_BASELINE_COUNT samples (1 s at 10 ms). */
            ctx->baseline_sum += (uint32_t)deci_hpa;
            ctx->baseline_count++;

            if (ctx->baseline_count >= BL_BASELINE_COUNT)
            {
                ctx->baseline = (int32_t)(ctx->baseline_sum / (uint32_t)BL_BASELINE_COUNT);
                /* Seed the drift accumulator so acc/1024 == baseline immediately. */
                ctx->acc      = (int64_t)ctx->baseline * 1024;
                ctx->state    = BL_IDLE;
            }
            return BL_EV_NONE;
        }

        case BL_IDLE:
        {
            /* Slow drift tracking: acc += p - acc/1024 */
            ctx->acc += (int64_t)deci_hpa - (ctx->acc / 1024);
            ctx->baseline = (int32_t)(ctx->acc / 1024);

            int32_t diff = deci_hpa - ctx->baseline;

            if (diff >= ctx->puff_threshold)
            {
                ctx->state = BL_PUFF;
                return BL_EV_PUFF_START;
            }
            if (-diff >= ctx->sip_threshold)
            {
                ctx->state = BL_SIP;
                return BL_EV_SIP_START;
            }
            return BL_EV_NONE;
        }

        case BL_PUFF:
        {
            /* Baseline does NOT drift during a puff. */
            int32_t diff = deci_hpa - ctx->baseline;
            int32_t drop_threshold = (ctx->puff_threshold * 6) / 10;

            if (diff < drop_threshold)
            {
                ctx->state = BL_IDLE;
                return BL_EV_PUFF_END;
            }
            return BL_EV_NONE;
        }

        case BL_SIP:
        {
            /* Baseline does NOT drift during a sip. */
            int32_t diff = ctx->baseline - deci_hpa;
            int32_t drop_threshold = (ctx->sip_threshold * 6) / 10;

            if (diff < drop_threshold)
            {
                ctx->state = BL_IDLE;
                return BL_EV_SIP_END;
            }
            return BL_EV_NONE;
        }

        default:
            return BL_EV_NONE;
    }
}

bool bl_sensor_ok(const bl_ctx_t *ctx)
{
    return ctx->fail_streak < 10u;
}

void bl_feed_fail(bl_ctx_t *ctx)
{
    if (ctx->fail_streak < 10u)
    {
        ctx->fail_streak++;
    }
}
