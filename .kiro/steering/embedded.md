---
inclusion: fileMatch
fileMatchPattern: "firmware/**"
---

# Embedded rules for the Blue Pill firmware

## Never

- Never call `HAL_Delay` inside the main loop or an interrupt. Delays are allowed only during start-up.
- Never use `malloc`, `free` or `printf` with floating point. All buffers are fixed-size and static.
- Never do work in an interrupt beyond recording a timestamp or setting a flag. The main loop does the rest.
- Never write outside `/* USER CODE BEGIN ... */` blocks in CubeMX-generated files. Put our code in `Core/Src/ditdah/` and `Core/Inc/ditdah/`.
- Never block waiting on I2C forever. Every I2C call uses a timeout of 10 ms or less and handles failure.

## Always

- Use the pin labels CubeMX generates in `main.h` (`BTN_BIG_Pin`, `MOTOR_Pin` and so on), never raw pin numbers.
- Read time with `HAL_GetTick()` (1 ms). Handle wrap-around by subtracting unsigned values.
- Debounce every switch for 10 ms before passing it to the decoder.
- Treat inputs as active-low: pressed reads `GPIO_PIN_RESET`.
- Keep RAM use under 16 KB of the 20 KB available, so the stack has room.
- Log over USART1 (115200 baud) with short lines, prefixed with the module name, such as `[mprls] 1013.2 hPa`.

## USB keyboard

- The device is a standard boot-protocol keyboard: 8-byte reports (modifier, reserved, 6 key codes).
- Every key press is sent as a press report followed by an all-zero release report.
- Wait for the previous report to finish before sending the next one; queue characters in a small ring buffer.
- F13 (HID usage 0x68) means "accept the suggestion" to the Studio.

## Flash settings

- Settings (speed, mode, sensitivity, feedback on or off) live in the last 1 KB flash page at `0x0800FC00`.
- Write them only when a setting changes, never on a timer.
