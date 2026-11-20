# Requirements Document

## Introduction

The decoder turns key presses into text. It is the heart of DitDah: the same rules run in the web Studio (TypeScript) and on the Blue Pill (C), and both must give the same answer for the same presses. People using DitDah may press slowly, unevenly or with long pauses, so the decoder must be forgiving of human timing while staying predictable.

A **unit** is the length of one dit. At the default speed of 10 words per minute (WPM), one unit is 120 ms (WPM = 1200 / unit in ms).

## Requirements

### Requirement 1: Read presses as dits and dahs (single-switch mode)

**User Story:** As a person typing with one switch, I want short presses read as dits and long presses read as dahs, so that I can spell any letter with one button.

#### Acceptance Criteria

1. WHEN a press lasts less than 2 units THEN the decoder SHALL record a dit.
2. WHEN a press lasts 2 units or longer THEN the decoder SHALL record a dah.
3. WHEN no unit is configured THEN the decoder SHALL use 120 ms.
4. WHEN a unit is configured THEN the decoder SHALL accept values from 48 ms to 200 ms and clamp anything outside that range.

### Requirement 2: Pauses end letters and words

**User Story:** As a person typing, I want a short pause to finish a letter and a longer pause to finish a word, so that I never need extra presses for spacing.

#### Acceptance Criteria

1. WHEN the silence after a press is shorter than 2 units THEN the decoder SHALL treat the next press as part of the same letter.
2. WHEN the silence reaches 2 units THEN the decoder SHALL end the current letter and output it.
3. WHEN the silence reaches 5 units AND a letter has been output since the last space THEN the decoder SHALL output a space.
4. The decoder SHALL never output two spaces in a row, and SHALL never output a space before the first letter.

### Requirement 3: Alphabet

**User Story:** As a person typing, I want letters, digits and common punctuation, so that I can write ordinary messages.

#### Acceptance Criteria

1. The decoder SHALL recognize the international Morse code for A–Z, 0–9 and the punctuation `. , ? ' / - @`.
2. WHEN a letter's pattern is not in the alphabet or the special codes THEN the decoder SHALL report it as unknown and SHALL NOT type anything for it.
3. WHEN a pattern is longer than 8 elements THEN the decoder SHALL report it as unknown.

### Requirement 4: Special codes

**User Story:** As a person typing, I want codes for backspace, Enter and accepting a suggestion, so that I can fix mistakes and type faster without another switch.

#### Acceptance Criteria

1. WHEN the pattern is 8 dits (`........`) THEN the decoder SHALL output Backspace.
2. WHEN the pattern is `.-.-` THEN the decoder SHALL output Enter.
3. WHEN the pattern is `..--` THEN the decoder SHALL output Accept (the Studio's "use the first suggestion").
4. WHEN Enter or Accept is output THEN the decoder SHALL treat the next word as new, so no space is added before it.

### Requirement 5: Two-switch mode

**User Story:** As a person who can control two switches, I want one switch for dit and one for dah, so that I don't have to judge how long I press.

#### Acceptance Criteria

1. WHILE in two-switch mode, WHEN the dit key is pressed THEN the decoder SHALL record a dit regardless of press length.
2. WHILE in two-switch mode, WHEN the dah key is pressed THEN the decoder SHALL record a dah regardless of press length.
3. WHILE in two-switch mode, the decoder SHALL apply the same pause rules as Requirement 2.
4. Sip and puff SHALL use two-switch mode, with a sip as the dit key and a puff as the dah key.

### Requirement 6: Live use on a device

**User Story:** As the firmware, I want to feed key-down, key-up and clock-tick events, so that letters appear while the person is still typing, not only at the end.

#### Acceptance Criteria

1. The decoder SHALL accept key-down and key-up events with a timestamp in milliseconds.
2. WHEN the clock is ticked during silence THEN the decoder SHALL output any letter or space whose pause threshold has been reached.
3. WHEN a key goes down after a pause long enough to end a letter or word THEN the decoder SHALL output that letter or space first, even if no tick arrived in between.
4. WHEN flush is called THEN the decoder SHALL output the current letter, if any, without a space.
5. The decoder SHALL handle the millisecond clock wrapping around (32-bit unsigned).

### Requirement 7: Adapt to each person's speed

**User Story:** As a person who presses faster or slower than the default, I want the decoder to learn my speed, so that I don't have to change settings.

#### Acceptance Criteria

1. WHILE adaptive mode is on, the decoder SHALL update its unit from the silences inside letters.
2. WHILE adaptive mode is on, the unit SHALL stay between 48 ms (25 WPM) and 200 ms (6 WPM).
3. WHEN someone types at a steady speed between 6 and 20 WPM, starting from the default unit, THEN by the third repetition of the word PARIS the decoder SHALL decode it correctly and its unit SHALL be within 15% of the true unit.
4. WHILE adaptive mode is off, the unit SHALL never change.

### Requirement 8: One behavior, two implementations

**User Story:** As the team, we want the web and device decoders to behave the same, so that practice on the web carries over to the device.

#### Acceptance Criteria

1. The TypeScript and C decoders SHALL both pass every case in `spec/morse-vectors.json` (the C runner reads `spec/morse_vectors.h`).
2. WHEN a rule changes THEN the generator `spec/gen-vectors.mjs`, both decoders and this document SHALL change together.

### Requirement 9: Forgiving and safe

**User Story:** As a person with uneven timing, I want small timing errors ignored, and as the team we want the decoder never to crash.

#### Acceptance Criteria

1. WHEN every press and pause is off by up to 25% from ideal timing THEN the decoder SHALL still produce the intended text.
2. WHEN given any sequence of events, including zero-length presses, negative or huge durations and repeated key-ups, THEN the decoder SHALL not throw, crash or write outside its buffers.
3. The C decoder SHALL use no dynamic memory and no more than 64 bytes of state.
