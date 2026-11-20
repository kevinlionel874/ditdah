# Wiring

Everything runs on 3.3 V from the Blue Pill. While developing, the ST-Link powers the board; in normal use the Blue Pill's own USB port does. **Never connect the ST-Link's 3.3V wire and the Blue Pill's USB at the same time.**

All switch inputs use the STM32's internal pull-ups and are pressed when pulled to GND, so no resistors are needed on them.

## Pin map

| Part | Part pin | Blue Pill pin | Notes |
|---|---|---|---|
| USB keyboard | — | PA11 / PA12 | The Micro-USB port. See "If the computer doesn't see DitDah" below |
| ST-Link | SWDIO / SWCLK / GND / 3.3V | DIO / DCLK / GND / 3.3 | 4-pin header at the end of the board |
| CP2102 (debug log) | TXD / RXD / GND | A10 / A9 / G | Don't connect the CP2102's 3V3 or 5V pins |
| OLED SH1106 (0x3C) | GND / VCC / SCL / SDA | G / 3.3 / B6 / B7 | Pin order differs between modules: follow the labels |
| MPRLS sensor (0x18) | VIN / GND / SCL / SDA | 3.3 / G / B6 / B7 | Shares B6 and B7 with the OLED; leave 3Vo, RST and EOC open |
| Jack 1 (big button or any switch) | T / S | A0 / G | |
| Jack 2 (external dit switch) | T / S | A1 / G | |
| Jack 3 (external dah switch) | T / S | A2 / G | |
| 30mm Dit button | NO / COM | B12 / G | |
| 30mm Dah button | NO / COM | B13 / G | |
| 30mm Mode button | NO / COM | B14 / G | |
| Vibration motor | through the transistor circuit | B0 | See below |
| Passive buzzer | + / − | A8 through 100 Ω / G | |
| Status LED | on the board | C13 | Lights when the pin is low |

## Jacks and switch cables

The 3.5mm jacks (PJ-320) are three-pole (TRS). The switch cables are four-pole (TRRS) plugs with bare wires. We measured both with a multimeter:

| TRRS cable wire | Plug section | Touches in the jack |
|---|---|---|
| **red** | tip | **T** |
| **white** | ring next to the sleeve | **S** |
| black | sleeve | nothing |
| fourth wire | first ring | nothing |

So a switch on a TRRS cable uses **red and white**:

- Switch **NO** → cable **red**
- Switch **COM** → cable **white**
- Black and the fourth wire are cut short and insulated

Each jack has a blue jumper on **T** (to A0, A1 or A2) and a black jumper on **S** (to G). Any ordinary mono 3.5mm assistive switch also works, because its long sleeve touches the jack's S contact.

## 30mm buttons

Each button's microswitch has a red jumper on **NO** and a white jumper on **COM**. Only the pair that connects when pressed is used. Red goes to B12, B13 or B14; white goes to G.

## Big button

The 100mm button's microswitch has four tabs: two thin tabs (**COM** and **NO**) are the switch, and two wide tabs with holes are for the built-in lamp. Only COM and NO are used; the lamp is not connected. The switch must click when the big button is pressed: if it doesn't, turn the microswitch in its socket until it does.

## Vibration motor

A pin can supply only about 20 mA and the motor needs 60–90 mA, so B0 switches it through an NPN transistor:

```
3.3V ──────┬───────────────┐
           │               │
     motor red (+)    1N4007 (stripe end)
           │               │
     motor black ──────────┴──── 2N2222 collector (C)
                                 2N2222 base (B) ── 1 kΩ ── B0
                                 2N2222 emitter (E) ── G
```

- 2N2222 with the flat, printed side facing you and the legs down: **E, B, C** from left to right (P2N2222A is the reverse: C, B, E).
- The diode's stripe goes to 3.3V. If it is reversed, the transistor shorts 3.3V to ground when it turns on.
- Test before connecting B0: touch the free end of the 1 kΩ to 3.3V and the motor should buzz.

## If the computer doesn't see DitDah

Many Blue Pills have the wrong pull-up resistor on USB D+ (R10 is 10 kΩ or 4.7 kΩ instead of 1.5 kΩ). If a computer doesn't recognize the board, solder a 2 kΩ resistor (anything from 1.5 to 2.2 kΩ) between **A12** and **3.3**.
