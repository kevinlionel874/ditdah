# DitDah

**Type with one button.** DitDah is an open-source assistive keyboard that uses Morse code. Anyone who can press one thing — a big button by hand, the same button on the floor with a foot, two switches, any switch with a 3.5mm plug, or a sip and a puff on a tube — can type on any computer, with no driver and no special software.

Commercial switch-access systems are expensive and often tied to one app. Morse code is from the 1840s, and it is still one of the fastest ways to type when you can reliably control only one or two movements. DitDah pairs it with cheap parts (about $90 for the prototype) and a free web app that predicts words and speaks text aloud.

> **Status:** early prototype, under active development. The first complete version is due in early December 2026.
> DitDah is **not a medical device**. See [Safety](#safety).

## How it works

```
 any input  ──►  DitDah device (STM32 Blue Pill)  ──USB keyboard──►  any app on any computer
 button, foot,     decodes Morse, vibrates and beeps                    └─► DitDah Studio (web):
 two switches,     on each dit and dah, shows the code                       AI word suggestions,
 sip and puff      on a small screen                                         speech, practice games
```

1. **DitDah device** — an STM32F103 "Blue Pill" reads the inputs, decodes Morse on the board and types the result as a standard USB keyboard. No driver, no app, works everywhere.
2. **Decoder** — one set of Morse rules, written twice (TypeScript for the web, C for the device) and tested against the same 69 shared test cases, so both always agree.
3. **DitDah Studio** — a web app that receives DitDah's keystrokes (or the spacebar, for anyone without the device), suggests the next word with AI so you press less, reads your text aloud, and teaches Morse with short games.

## Morse rules

| | |
|---|---|
| Speed | Starts at 10 words per minute (one unit = 120 ms) and adapts to each person, from 6 to 25 WPM |
| Dit or dah | A press shorter than 2 units is a dit; 2 units or longer is a dah |
| End of letter | 2 units of silence |
| Space | 5 units of silence (never two spaces in a row) |
| Backspace | `........` (8 dits) |
| Enter (speak the line) | `.-.-` |
| Accept the AI suggestion | `..--` (the device sends F13) |
| Two-switch and sip-and-puff | One switch (or a sip) is dit, the other (or a puff) is dah; only the pauses are timed |

## Repository

| Folder | What's in it |
|---|---|
| `.kiro/steering/` | Product, tech, embedded and structure rules that Kiro follows |
| `.kiro/specs/` | Specs for the decoder, the firmware and the Studio (requirements, design, tasks) |
| `spec/` | The shared Morse test cases and the script that generates them |
| `decoder/` | The decoder: TypeScript in `src/`, C in `c/`, tests in `test/` |
| `firmware/` | The STM32 firmware (CubeMX project; our code is in `Core/Src/ditdah/`) |
| `studio/` | The Next.js web app |
| `docs/` | Wiring, CubeMX setup, Kiro hooks, and notes on how Kiro helped |

## Try it

### Studio (no hardware needed)

```bash
cd studio
npm install
npm run dev
```

Open http://localhost:3000, hold the spacebar to type. AI suggestions need `AI_API_KEY`, `AI_BASE_URL` and `AI_MODEL` (any OpenAI-compatible API) in `studio/.env.local`; without them the Studio uses a built-in word list.

### Decoder tests

From the repo root:

```bash
(cd decoder && npm install && npm test)
(cd decoder/c && make test)
```

To regenerate the shared test cases: `node spec/gen-vectors.mjs`.

### Firmware

You need STM32CubeIDE 2.x (its bundled CMake, Ninja and GCC are used), an ST-Link V2 and, for the debug log, a CP2102 USB-serial adapter. On Windows PowerShell:

```powershell
cd firmware
.\build.ps1      # build\Debug\firmware.elf
.\flash.ps1      # write it through the ST-Link
.\monitor.ps1    # show the debug log (115200 baud)
```

Details: [docs/cubemx-setup.md](docs/cubemx-setup.md). Wiring: [docs/wiring.md](docs/wiring.md).

## Hardware

About $93 of new parts, plus things we already had.

| Part | Use |
|---|---|
| STM32F103C8T6 Blue Pill, ST-Link V2, CP2102 | Main board, flashing, debug log |
| 1.3" SH1106 OLED (I2C) | Shows the code being entered |
| Adafruit MPRLS ported pressure sensor + 2mm silicone tubing | Sip and puff |
| 100mm arcade button | Big switch for hand or foot |
| 3 × 30mm arcade buttons | Dit, dah and mode on the case |
| 3 × 3.5mm jacks (PJ-320) + TRRS cables | Plug in the big button or anyone's own switches |
| Coin vibration motor, 2N2222, 1N4007, 1 kΩ | Feel each dit and dah |
| Passive buzzer | Hear each dit and dah |

## Device and Studio together

DitDah is a plain USB keyboard, so the Studio needs no special connection: it just receives letters, spaces, Backspace and Enter. One extra key, **F13**, means "accept the suggestion". In the Studio, switch to **device mode** so the spacebar types a normal space.

## Roadmap

The first version is a USB keyboard on a breadboard in a 3D-printed box, plus the web Studio. Next we want a cheaper custom board, a wireless version for phones and tablets, word prediction that works offline, and testing with people who use switches every day. See [docs/roadmap.md](docs/roadmap.md).

## Contributing

Issues and pull requests are welcome, especially from people who use switch access, occupational therapists, and caregivers: tell us what gets in the way. Each part has a spec in `.kiro/specs/` (requirements, design and tasks) that explains how it is meant to work; start there. Please run the tests for the part you change before opening a pull request.

## How it started

DitDah started at [Kiroween 2026](https://kiroween.devpost.com), a hackathon run by Kiro, in the Resurrection category (bringing an old technology back to life). We are continuing it after the hackathon.

## Built with Kiro

We used Kiro's steering docs to hold the rules every change must follow (for example: no blocking delays in the firmware main loop, and the TypeScript and C decoders must stay identical), specs to break each part into requirements, a design and a task list, hooks to run the tests on every save, and property-based tests to check the decoder against human timing. Notes and screenshots of where this caught real problems are in `docs/kiro-moments/`.

We also used Claude (an AI assistant) to help draft the specs and test cases and to review the generated code. We say which parts in the Devpost write-up.

## Safety

- DitDah is a prototype, not a medical or certified assistive device. Don't rely on it as someone's only way to call for help.
- Each sip-and-puff user needs their own mouthpiece. Never share one.

## Team

[kevinlionel874](https://github.com/kevinlionel874) (hardware and firmware), [Gall1um](https://github.com/Gall1um) (testing, video, Devpost), [winter-soldier25](https://github.com/winter-soldier25) (software).

## License

[MIT](LICENSE)
