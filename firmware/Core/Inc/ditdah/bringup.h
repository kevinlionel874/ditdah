/* Hardware bring-up test. Checks each part on its own before the real firmware:
 * - LED on PC13 blinks twice a second (the board is running)
 * - USART1 log at 115200: boot message, I2C scan, pressure 10 times a second, input changes
 * - OLED shows the I2C results, the pressure and which inputs are pressed
 * - pressing any input turns on the LED, the vibration motor (PB0) and the buzzer (PA8)
 * Call bringup_init() once after the CubeMX init functions, then bringup_loop() forever. */
#ifndef DITDAH_BRINGUP_H
#define DITDAH_BRINGUP_H

void bringup_init(void);
void bringup_loop(void);

#endif
