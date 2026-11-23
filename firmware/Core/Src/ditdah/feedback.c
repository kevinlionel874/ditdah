#include "ditdah/feedback.h"

#include <stdbool.h>
#include <stdint.h>

#include "main.h"
#include "tim.h"

/* --------------------------------------------------------------------------
 * Motor
 * -------------------------------------------------------------------------- */
static bool     s_motor_on    = false;
static uint32_t s_motor_until = 0u;

/* --------------------------------------------------------------------------
 * Buzzer / tone
 * -------------------------------------------------------------------------- */
static bool s_sound_enabled = true;
static bool s_tone_on       = false;   /* tracks whether TIM1 PWM is running */

/* --------------------------------------------------------------------------
 * Two-buzz state machine (for MD_OUT_UNKNOWN)
 *   0 = idle
 *   1 = first tone active
 *   2 = gap (silent)
 *   3 = second tone active
 * -------------------------------------------------------------------------- */
static uint8_t  s_buzz_state = 0u;
static uint32_t s_buzz_until = 0u;

#define BUZZ_TONE_MS 60u
#define BUZZ_GAP_MS  60u

/* Internal: start or stop TIM1 PWM without touching the buzz state machine. */
static void pwm_start(void)
{
    if (!s_tone_on)
    {
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
        s_tone_on = true;
    }
}

static void pwm_stop(void)
{
    if (s_tone_on)
    {
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
        s_tone_on = false;
    }
}

/* -------------------------------------------------------------------------- */

void feedback_init(void)
{
    s_motor_on      = false;
    s_motor_until   = 0u;
    s_sound_enabled = true;
    s_tone_on       = false;
    s_buzz_state    = 0u;
    s_buzz_until    = 0u;

    HAL_GPIO_WritePin(MOTOR_GPIO_Port, MOTOR_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_GPIO_Port,   LED_Pin,   GPIO_PIN_SET);  /* active-low: off */
    pwm_stop();
}

void feedback_motor_pulse(uint32_t ms)
{
    uint32_t deadline = HAL_GetTick() + ms;
    if (!s_motor_on || (int32_t)(deadline - s_motor_until) > 0)
    {
        s_motor_until = deadline;
    }
    if (!s_motor_on)
    {
        HAL_GPIO_WritePin(MOTOR_GPIO_Port, MOTOR_Pin, GPIO_PIN_SET);
        s_motor_on = true;
    }
}

void feedback_tone(bool on)
{
    /* While the buzz state machine is active, ignore external tone requests
     * so we don't interrupt the two-buzz sequence. */
    if (s_buzz_state != 0u)
    {
        return;
    }
    if (!s_sound_enabled)
    {
        pwm_stop();
        return;
    }
    if (on)
    {
        pwm_start();
    }
    else
    {
        pwm_stop();
    }
}

void feedback_led(bool on)
{
    /* Blue Pill LED is active-low on PC13 */
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,
                      on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void feedback_set_sound(bool enabled)
{
    s_sound_enabled = enabled;
    if (!enabled)
    {
        pwm_stop();
        s_buzz_state = 0u;
    }
}

void feedback_buzz_unknown(void)
{
    if (!s_sound_enabled)
    {
        return;
    }
    /* Start the two-buzz state machine */
    s_buzz_state = 1u;
    s_buzz_until = HAL_GetTick() + BUZZ_TONE_MS;
    pwm_start();
}

void feedback_poll(uint32_t now)
{
    /* Motor timeout */
    if (s_motor_on && ((int32_t)(now - s_motor_until) >= 0))
    {
        HAL_GPIO_WritePin(MOTOR_GPIO_Port, MOTOR_Pin, GPIO_PIN_RESET);
        s_motor_on = false;
    }

    /* Two-buzz state machine */
    if (s_buzz_state != 0u && ((int32_t)(now - s_buzz_until) >= 0))
    {
        switch (s_buzz_state)
        {
            case 1u:  /* first tone done → start gap */
                pwm_stop();
                s_buzz_state = 2u;
                s_buzz_until = now + BUZZ_GAP_MS;
                break;
            case 2u:  /* gap done → start second tone */
                if (s_sound_enabled)
                {
                    pwm_start();
                }
                s_buzz_state = 3u;
                s_buzz_until = now + BUZZ_TONE_MS;
                break;
            case 3u:  /* second tone done → idle */
                pwm_stop();
                s_buzz_state = 0u;
                break;
            default:
                s_buzz_state = 0u;
                break;
        }
    }
}
