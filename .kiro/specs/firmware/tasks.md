# Implementation Plan

Build with `firmware/build.ps1`, flash with `firmware/flash.ps1`, watch the log with `firmware/monitor.ps1`. Commit after each task, and save a screenshot of anything interesting to `docs/kiro-moments/`. Tasks that need the board say what to check on it.

- [ ] 1. Run the bring-up test on the board
  - Wire OLED, MPRLS and CP2102 as in `docs/wiring.md` and the ST-Link as in `docs/cubemx-setup.md`; flash the current firmware
  - On the board: LED blinks twice a second, the log shows `found 0x3C (OLED)` and `found 0x18 (MPRLS)`, blowing makes `d=` positive and sipping makes it negative, touching A0 to G logs `[input] BIG down`
  - _Requirements: 9.1_

- [ ] 2. Make the board a USB keyboard
  - Create `firmware/tools/patch-hid-keyboard.ps1` that applies the three HID changes in design.md and adds the `DITDAH_KEYBOARD_PATCH` marker; make `build.ps1` stop with a clear message when the marker is missing
  - Create `usb_reconnect.c` (PA12 low for 10 ms) and call it in `USER CODE BEGIN SysInit`
  - Create `keyboard.c` with the queue and the US character map from design.md
  - Temporary check in the bring-up loop: pressing the big button types `e`
  - On the board: `e` appears in Notepad on two different computers. If a computer doesn't see the device, add a 2 kΩ resistor from PA12 to 3.3V
  - _Requirements: 1.1–1.5, 8.2_

- [x] 3. Type with one button
  - Add `../decoder/c/morse_decoder.c` and the `../decoder/c` include path to `firmware/CMakeLists.txt`
  - Create `inputs.c` (10 ms debounce, first-edge timing) and `app.c` with one-button mode and the decoder output table; switch `main.c` from the bring-up test to `app_init`/`app_loop`
  - On the board: type HELLO WORLD into Notepad with only the big button
  - _Requirements: 2.1–2.3, 3.1, 4.1, 4.2, 4.4_

- [ ] 4. Add feedback and the display
  - Create `feedback.c` (motor pulses 40/120 ms, 600 Hz tone, LED) and `display.c` (the layout in design.md, one dirty line per pass)
  - Work out dit or dah at key-up in `app.c` and keep the pattern string
  - On the board: with eyes closed you can tell a dit from a dah by feel; the OLED shows the code, the last letter and the typed text
  - _Requirements: 4.3, 5.1–5.4_

- [ ] 5. Add two-switch mode and the mode button
  - Two-switch mode with `BTN_DIT`/`JACK_DIT` and `BTN_DAH`/`JACK_DAH` as in design.md; short press of `BTN_MODE` cycles the mode, a 2 s hold toggles sound
  - On the board: HELLO WORLD with the two 30mm buttons, and again with switches in jacks 2 and 3
  - _Requirements: 3.2, 3.4, 3.5_

- [ ] 6. Add sip and puff
  - Create `breath.c` with the state machine in design.md; split the threshold logic into `breath_logic.c` so it can be tested on the laptop
  - Record soft and hard puffs and sips from the log and set the default thresholds from them
  - On the board, with your own straw: SOS by breath alone, three times in a row
  - _Requirements: 3.3, 6.1–6.6_

- [ ] 7. Save settings in flash
  - Change the FLASH length in `STM32F103xx_FLASH.ld` to 63K; create `settings.c` as in design.md
  - On the board: change mode and speed, unplug, plug back in; the settings are still there
  - _Requirements: 7.1–7.4_

- [ ] 8. Make it reliable
  - Create `i2c_bus.c` with the recovery in design.md and report every OLED and sensor I2C result to it
  - On the board: unplug and replug USB 10 times; leave it running for an hour and type now and then; no freezes
  - Build Release (`.\build.ps1 Release`) and check it fits in 63 KB of flash and under 16 KB of RAM
  - _Requirements: 1.6, 8.1–8.4_

- [ ] 9. Host tests for the firmware logic
  - In `firmware/test/`, compile `keyboard_map.c` and `breath_logic.c` with gcc and test the character map (every character in Requirement 1.3) and the breath thresholds and hysteresis
  - Add a `make test` target like the decoder's
  - _Requirements: 1.3, 6.4_
