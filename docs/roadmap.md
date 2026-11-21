# Roadmap

DitDah is meant to outlive the hackathon it started in. This page lists what the first version covers and the ideas we want to explore next. Nothing below "Next" is a promise; open an issue if one of these matters to you, or if something more important is missing.

## Version 1 (December 2026)

- USB keyboard on an STM32 Blue Pill: one button, two switches, or sip and puff
- The shared Morse decoder in TypeScript and C, tested against the same cases
- Vibration, sound and a small screen for feedback
- Settings saved on the device
- DitDah Studio on the web: spacebar typing, AI word suggestions with an offline fallback, speech, a phrase board, practice games
- Breadboard inside a 3D-printed box

## Next

### Hardware

- **A custom circuit board** instead of the breadboard and the Blue Pill: jacks, buttons, motor driver, screen and sensor on one board that anyone can order and assemble. Target: under $40 in parts.
- **Wireless**: a Bluetooth keyboard version (for example on an ESP32-S3 or nRF52840) with a battery, so DitDah works with phones and tablets.
- **Better switch access**: more jack inputs, adjustable sensitivity, and testing with common commercial assistive switches.
- **Sip and puff that's easy to keep clean**: a replaceable mouthpiece and filter, and a mount for a desk or wheelchair.
- **Printable files**: publish the case and mount files so others can print their own.

### Firmware and Studio

- **Word prediction on the device**, so suggestions work with no computer app and no internet.
- **Mouse mode**: Morse codes that move the pointer and click.
- **Set up the device from the Studio** (speed, mode, sensitivity) over WebHID, instead of with buttons.
- **The Studio offline** as an installable web app, with personal phrase sets.
- **More languages**: other Morse alphabets and text-to-speech voices.
- **Ready-to-flash firmware** attached to each GitHub release, so nobody has to build it.

### People

- Try DitDah with occupational therapists and people who use switch access every day, and change the defaults (speed, timing tolerance, feedback) based on what they find.
- Plain-language setup guides and short videos for caregivers.

### Project

- A hardware license (such as CERN-OHL-P) for the board and case files, alongside the MIT license for the code.
- A contributing guide and issue templates.

## Limits we keep

DitDah is not a medical device. Any clinical use would need proper evaluation and certification, which is outside what a student project can do. We will keep saying so in the app and the docs.
