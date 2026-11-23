# Board test checklist

Use this the first time the firmware meets the real board, and again after any rewiring. It takes about two to three hours. Do the steps in order: each one only adds one new thing, so when a step fails you know where to look.

Every step has a **Pass** line. If a step doesn't pass, stop and fix it (the **If not** lines list the usual causes) before going on. Pin names are the labels printed on the Blue Pill (`A0`, `B12`, `3.3`, `G`). Full wiring: [`wiring.md`](wiring.md).

## What you need

- The Blue Pill on the breadboard, with `3.3` and `G` wired to the power rails
- A **data** Micro-USB cable (the package says "data" or "sync"; a charge-only cable powers the board but the computer never sees it)
- A USB hub with three USB-A ports, for the ST-Link, the CP2102 and the Blue Pill
- ST-Link V2 with 4 female-to-female jumpers, CP2102 with 3 jumpers
- Multimeter
- OLED, MPRLS sensor and tube, the big button (plugged into jack 1), the three 30mm buttons, jacks 2 and 3
- Motor circuit parts (2N2222, 1N4007, 1 kΩ), passive buzzer and 100 Ω, and a 2 kΩ resistor in case the computer doesn't see the board
- Your own straw for the breath test, never shared

**Power rule:** the ST-Link powers the board while you flash; the Micro-USB powers it while you use it. Never connect the ST-Link's 3.3V wire and the Micro-USB at the same time.

## 1. Power

Connect only the Micro-USB cable, nothing else.

- **Pass:** the red PWR LED is on. The `3.3` rail measures 3.2–3.4 V and `5V` measures 4.7–5.2 V. After a minute the regulator (the small 3-leg chip near the USB port) is at most warm. The green LED on C13 may blink; that's the factory test program.
- **If not:** try another cable and USB port first. With power off, measure resistance between `3.3` and `G`: close to 0 Ω means a short (look for solder bridges and a jumper in the wrong row).

Unplug the Micro-USB.

## 2. ST-Link, log cable and I2C parts

| From | To |
|---|---|
| ST-Link SWDIO / SWCLK / GND / 3.3V | Blue Pill `DIO` / `DCLK` / `G` / `3.3` (4-pin header at the end) |
| CP2102 TXD / RXD / GND | `A10` / `A9` / `G` (leave the CP2102's 3V3 and 5V pins unconnected) |
| OLED and MPRLS: SCL / SDA / VCC (VIN) / GND | `B6` / `B7` / `3.3` / `G` |

Plug the ST-Link and the CP2102 into the hub. The Blue Pill's Micro-USB stays unplugged.

## 3. Flash the bring-up test

The bring-up test checks the hardware, not the Morse code: it scans I2C, prints the pressure, and turns on the LED, motor and buzzer when any input is pressed.

```powershell
cd E:\DitDah\ditdah\firmware
.\build.ps1 -Bringup
.\flash.ps1
```

- **Pass:** the last lines say `Download verified successfully` and the board resets.
- **If not:** `No ST-LINK detected` means the driver (STSW-LINK009) or the USB port. `No STM32 target found` means DIO and DCLK are swapped, a wire is loose, or the BOOT0 jumper isn't on 0.

## 4. Read the log

Open a second PowerShell window:

```powershell
cd E:\DitDah\ditdah\firmware
.\monitor.ps1
```

Press the Blue Pill's RESET button.

- **Pass:** you see `[boot] DitDah bring-up test, SYSCLK 72000000 Hz`, then `[i2c] found 0x18`, `[i2c] found 0x3C`, `[i2c] 2 device(s)`, `[oled] ready`, `[boot] ready`, and after a second `[mprls] baseline ... hPa`. The OLED shows `DitDah bring-up`.
- **If not:** no log at all means A9 and A10 are swapped or the wrong COM port (`.\monitor.ps1 -Port COM5`). `[i2c] nothing found` means SCL and SDA are swapped or the parts have no 3.3 V. Only one address found: check that part's four wires.

## 5. Breath readings

Fit your own straw to the tube and the tube to the MPRLS port. Watch the `d=` value (the difference from the baseline, in hPa) as you:

| Action | Write down the biggest `d=` |
|---|---|
| Soft puff | |
| Hard puff | |
| Soft sip | |
| Hard sip | |

- **Pass:** puffs make `d` positive, sips make it negative, and it returns near 0 when you stop.
- Send the four numbers to the group. The firmware starts a puff or sip at 10.0 hPa by default; if your soft puff can't reach that, the threshold goes down.

## 6. Inputs

| Input | Wire it to |
|---|---|
| Jack 1 (big button) | T to `A0`, S to `G` |
| Jack 2 / jack 3 | T to `A1` / `A2`, S to `G` |
| 30mm buttons dit / dah / mode | NO to `B12` / `B13` / `B14`, COM to `G` |

Press and release each input once.

- **Pass:** the log shows `[input] BIG down` and `[input] BIG up` (then `JDIT`, `JDAH`, `DIT`, `DAH`, `MODE`), and the C13 LED lights while anything is pressed.
- **If not:** `is already down at start` means that pin is wired to `G` or the switch is stuck. Nothing at all means a loose jumper or the wrong pin.

## 7. Motor and buzzer

- Buzzer: `A8` through 100 Ω to the buzzer's +, its − to `G`.
- Motor: build the circuit in [`wiring.md`](wiring.md#vibration-motor) (red to `3.3`, blue to the 2N2222 collector, diode stripe to `3.3`). Before connecting `B0`, touch the free end of the 1 kΩ to `3.3`: the motor should buzz. Then connect it to `B0`.

- **Pass:** holding any input runs the motor and sounds the buzzer; releasing stops both.
- **If not:** the transistor gets hot means the diode is backwards or E and C are swapped. Unplug at once.

## 8. USB keyboard

Unplug the ST-Link's 3.3V wire from the Blue Pill (the other three can stay). Plug the Blue Pill's Micro-USB into the hub. Open Notepad.

- **Pass:** Windows plays the connect sound, Device Manager shows a new keyboard, and pressing the big button types `e`. If you can, try a second computer too.
- **If not:** "USB device not recognized" usually means the wrong pull-up resistor on the board. Put the 2 kΩ resistor from the `A12` row to the 3.3V rail and replug (see [`wiring.md`](wiring.md#if-the-computer-doesnt-see-ditdah)).

## 9. Flash the DitDah app

Unplug the Micro-USB and reconnect the ST-Link's 3.3V wire.

```powershell
.\build.ps1
.\flash.ps1
```

Unplug the ST-Link's 3.3V wire again and plug in the Micro-USB.

- **Pass:** the log shows `[app] init done, 1-btn mode` and `[breath] sensor ok`. The OLED shows `DitDah 10wpm 1-btn`, `Code:`, `Last:` and `USB ok`.

## 10. HELLO WORLD with one button

At the starting speed (10 WPM): a **dit** is a quick tap (under a quarter of a second), a **dah** is a hold of about half a second. Keep the gaps inside a letter short, pause about half a second between letters, and a full second or a little more between words. The speed adapts to you after a few letters.

| H | E | L | L | O | | W | O | R | L | D |
|---|---|---|---|---|---|---|---|---|---|---|
| `....` | `.` | `.-..` | `.-..` | `---` | pause | `.--` | `---` | `.-.` | `.-..` | `-..` |

- **Pass:** Notepad shows `hello world`. You hear the tone while pressing, feel a short buzz after a dit and a long one after a dah, and the OLED's `Code:` line shows the dots and dashes. Film it for 10 seconds; this is milestone M1.
- **If not:** a letter splits into two (H comes out as `ee`) means the pauses inside it were too long. Two letters merge into one means the pause between them was too short. `[morse] unknown` in the log means the pattern wasn't a letter.

Also try the special codes: `.-.-` is Enter and `........` (8 dits) is Backspace. `..--` sends F13, which Notepad ignores; the Studio uses it.

## 11. Two-switch mode

Press the mode button briefly: the OLED shows `2-sw`. Now the dit button types dits and the dah button types dahs, however long you hold them.

- **Pass:** SOS (`...` `---` `...`) types `sos`, first with the 30mm buttons, then with switches plugged into jacks 2 and 3.
- Hold the mode button for 2 seconds: the status line shows `snd off` and the tone stops. Hold it again to turn sound back on.

## 12. Sip and puff

Press the mode button briefly again: the OLED shows `breath`. Keep still for one second while it measures the baseline. A sip is a dit and a puff is a dah.

- **Pass:** SOS by breath (three sips, pause, three puffs, pause, three sips) types `sos`, three times in a row.
- **If not:** if nothing happens, your puffs may be below the 10.0 hPa threshold; send the numbers from step 5. `No breath sensor` on the status line means the MPRLS stopped answering: check its wires.

## 13. Quick reliability check

- Unplug and replug the Micro-USB five times; it should type every time without a reset.
- Leave it running for 30 minutes and type now and then.

## What to send the group

- A screenshot of the log from step 4
- The four breath numbers from step 5
- The 10-second HELLO WORLD video from step 10
- Anything that failed: the step number, what you saw, and a photo of the wiring
