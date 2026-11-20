---
inclusion: always
---

# Product: DitDah

DitDah turns Morse code into an assistive keyboard. Anyone who can press one thing (a big button, a foot switch, any switch with a 3.5mm plug, or a sip and puff on a tube) can type on any computer.

## Who it's for

- People with severe motor disabilities, such as ALS, cerebral palsy or spinal cord injury, who can reliably control only one or two switches.
- Caregivers, occupational therapists and family members who set the device up.
- Hackathon judges, who will mostly try the web Studio with a spacebar and watch the hardware in the demo video.

## The three parts

1. **DitDah device**: an STM32F103 "Blue Pill" that reads the inputs, decodes Morse on the device, and types the result as a standard USB keyboard. It needs no driver and works in any app.
2. **Decoder**: one set of Morse rules, implemented twice (TypeScript for the web, C for the device), tested against the same shared test cases.
3. **DitDah Studio**: a web app that receives DitDah's keystrokes (or the spacebar), suggests the next word or phrase with AI, speaks text aloud, and teaches Morse with practice games.

## Principles

- **Accessibility first.** Large text, high contrast, everything usable by keyboard alone, and nothing that requires fine motor control.
- **Fewer presses.** Every feature is judged by whether it reduces the number of dits and dahs a person needs.
- **Works offline.** The device types without the Studio, and word completion falls back to a built-in list when AI is unavailable.
- **Honest limits.** DitDah is a student prototype, not a medical device. Say so in the UI and docs.
- **Hygiene.** Sip-and-puff users each use their own mouthpiece; never suggest sharing one.

## Out of scope for the hackathon

- Bluetooth or wireless connections
- Phone apps
- Languages other than English for the Morse alphabet
