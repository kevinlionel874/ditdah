#ifndef DITDAH_FEEDBACK_H
#define DITDAH_FEEDBACK_H

#include <stdbool.h>
#include <stdint.h>

void feedback_init(void);
void feedback_motor_pulse(uint32_t ms);  /* extends current pulse */
void feedback_tone(bool on);             /* 600 Hz on TIM1 CH1; ignored if sound off */
void feedback_led(bool on);              /* LED_Pin on GPIOC; active-low */
void feedback_poll(uint32_t now);        /* turns off motor and advances buzz state machine */
void feedback_set_sound(bool enabled);   /* called from app when sound is toggled */
void feedback_buzz_unknown(void);        /* two 60 ms tones with a 60 ms gap */

#endif /* DITDAH_FEEDBACK_H */
