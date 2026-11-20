# Requirements Document

## Introduction

The DitDah firmware runs on an STM32F103C8 Blue Pill. It reads a switch, two switches, or breath pressure, decodes Morse with the shared C decoder, and types the result into any computer as a standard USB keyboard. An OLED, a vibration motor and a buzzer tell the user what was entered.

Pins, peripherals and labels come from `firmware/firmware.ioc` (see `docs/cubemx-setup.md`). The decoder rules are in `.kiro/specs/decoder/`. The embedded rules in `.kiro/steering/embedded.md` apply to every task.

## Requirements

### Requirement 1: Be a USB keyboard

**User Story:** As a DitDah user, I want the device to work as a normal keyboard, so that I can type in any app on any computer without installing anything.

#### Acceptance Criteria

1. WHEN DitDah is plugged into Windows, macOS or Linux THEN it SHALL enumerate as a standard boot-protocol keyboard named "DitDah Keyboard" with no driver install.
2. WHEN a character is decoded THEN the firmware SHALL send one key-press report followed by one all-keys-released report.
3. The firmware SHALL type lowercase letters, digits, space, and `. , ? ' / - @` using a US keyboard layout, adding Shift where the character needs it.
4. The firmware SHALL send Backspace for the backspace code, Enter for the enter code, and F13 (HID usage 0x68) for the accept code.
5. WHEN characters are decoded faster than USB can send them THEN the firmware SHALL queue at least 32 of them in order and SHALL drop new ones, with a log line, only when the queue is full.
6. WHEN USB is unplugged and plugged back in THEN the device SHALL be typing again within 3 seconds without a reset.

### Requirement 2: Read the switches

**User Story:** As a user with my own switch, I want every press counted once and timed accurately, so that my dits and dahs come out right.

#### Acceptance Criteria

1. The firmware SHALL read the six inputs `BTN_BIG`, `JACK_DIT`, `JACK_DAH`, `BTN_DIT`, `BTN_DAH` and `BTN_MODE` as active-low with internal pull-ups.
2. The firmware SHALL debounce every input for 10 ms and SHALL time each press from its first edge, so debouncing does not change the measured length.
3. The firmware SHALL check the inputs at least once per millisecond on average.

### Requirement 3: Three ways to type

**User Story:** As a user, I want to choose the input that suits my body, so that I can type with a hand, a foot, two switches or my breath.

#### Acceptance Criteria

1. WHILE in one-button mode THE firmware SHALL feed `BTN_BIG` to the decoder in single mode.
2. WHILE in two-switch mode THE firmware SHALL feed `BTN_DIT` or `JACK_DIT` as the dit key and `BTN_DAH` or `JACK_DAH` as the dah key, with the decoder in dual mode.
3. WHILE in sip-and-puff mode THE firmware SHALL feed a sip as the dit key and a puff as the dah key, with the decoder in dual mode.
4. WHEN `BTN_MODE` is pressed for less than 1 second THEN the firmware SHALL move to the next mode (one-button → two-switch → sip-and-puff → one-button), reset the decoder, and show the new mode.
5. WHEN `BTN_MODE` is held for 2 seconds or longer THEN the firmware SHALL toggle the sound on or off.

### Requirement 4: Decode Morse

**User Story:** As a user, I want the device to follow the same rules as the Studio, so that what I learn in practice works on the device.

#### Acceptance Criteria

1. The firmware SHALL use the C decoder in `decoder/c/` unchanged, built into the firmware from that folder, not copied.
2. The firmware SHALL call `md_tick` at least every 10 ms.
3. WHEN the decoder outputs an unknown pattern THEN the firmware SHALL type nothing, show `?` on the OLED, and give two short buzzes.
4. The firmware SHALL start at the saved speed and SHALL use adaptive speed unless it is turned off in the settings.

### Requirement 5: Feedback

**User Story:** As a user who may not be able to see the screen or hear well, I want to feel and hear what I entered, so that I know whether it was a dit or a dah.

#### Acceptance Criteria

1. WHEN the decoder records a dit THEN the motor SHALL run for 40 ms; WHEN it records a dah THEN the motor SHALL run for 120 ms.
2. WHILE a key is down (or a sip or puff is active) AND sound is on THE buzzer SHALL play a 600 Hz tone.
3. The OLED SHALL show the mode, the speed in WPM, the dits and dahs of the letter in progress, the last letter, and the last 21 characters typed.
4. The OLED SHALL update without blocking the main loop for more than 15 ms at a time.

### Requirement 6: Sip and puff

**User Story:** As a user who can't press a switch, I want to type by breathing into a tube, so that I can type with no movement at all.

#### Acceptance Criteria

1. The firmware SHALL read the pressure sensor every 10 ms without blocking.
2. WHEN the device starts THEN the firmware SHALL measure the resting pressure for 1 second and use it as the baseline.
3. WHILE no sip or puff is active THE firmware SHALL let the baseline follow slow drift (time constant of about 10 seconds).
4. WHEN the pressure rises above the baseline by the puff threshold THEN a puff SHALL start; WHEN it falls back below 60% of that threshold THEN the puff SHALL end. Sips work the same way below the baseline.
5. The puff and sip thresholds SHALL start at 10 hPa and SHALL be adjustable in the saved settings.
6. IF the sensor stops answering THEN the firmware SHALL show "No breath sensor", log it, and keep the other modes working.

### Requirement 7: Saved settings

**User Story:** As a user or caregiver, I want my speed and mode remembered, so that I don't set them up every time.

#### Acceptance Criteria

1. The firmware SHALL store mode, speed, adaptive on or off, sound on or off, motor on or off, and the sip and puff thresholds in the last 1 KB flash page at `0x0800FC00`, with a version number and a checksum.
2. The linker script SHALL keep program code out of that page.
3. WHEN the device starts AND the stored settings are missing or the checksum is wrong THEN the firmware SHALL use the defaults.
4. The firmware SHALL write the page only when a setting changes, at most once every 5 seconds.

### Requirement 8: Reliability

**User Story:** As a user who depends on DitDah to communicate, I want it to keep working, so that it doesn't freeze in the middle of a conversation.

#### Acceptance Criteria

1. IF an I2C call fails three times in a row THEN the firmware SHALL run a bus recovery (clock out SCL nine times, then re-initialize I2C1) and log it.
2. WHEN the board resets THEN the firmware SHALL pull USB D+ (PA12) low for 10 ms before USB starts, so the computer sees a fresh connection.
3. The firmware SHALL run for an hour of occasional typing without a freeze or a missed press.
4. The firmware SHALL use no more than 16 KB of RAM, and the Release build SHALL fit in 63 KB of flash.

### Requirement 9: Debug log

**User Story:** As the developer, I want to see what the firmware is doing, so that I can fix problems quickly.

#### Acceptance Criteria

1. The firmware SHALL log over USART1 at 115200 baud, one short line per event, prefixed with the module name.
2. The firmware SHALL log start-up, mode changes, each decoded output, USB connect and disconnect, sensor errors and I2C recoveries.
3. The firmware SHALL NOT use floating point in log formatting.
