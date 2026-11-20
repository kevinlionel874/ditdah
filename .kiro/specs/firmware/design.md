# Design Document

## Overview

The firmware is the CubeMX project in `firmware/` (CMake, built with `firmware/build.ps1`). CubeMX owns the generated files; our code lives in `firmware/Core/Src/ditdah/` and `firmware/Core/Inc/ditdah/` and is called from three `USER CODE` blocks in `main.c`:

| Block | Call |
|---|---|
| `USER CODE BEGIN SysInit` | `usb_force_reconnect()` (PA12 low for 10 ms) |
| `USER CODE BEGIN 2` | `app_init()` |
| `USER CODE BEGIN 3` | `app_loop()` |

Today these call the bring-up test (`bringup.c`). Task 3 switches `main.c` to `app_init`/`app_loop`; `bringup.c` stays in the repo for hardware checks.

Everything runs in the main loop. There are no interrupts of our own: inputs are polled, the I2C sensor is read with a non-blocking state machine, and USB reports are sent when the previous one has finished. The SysTick (1 ms, `HAL_GetTick()`) is the only clock.

## Modules

| File | Job |
|---|---|
| `app.c` | Owns the mode, the decoder and the settings; wires the other modules together |
| `inputs.c` | Debounces the six switches; produces press and release events with the time of the first edge |
| `breath.c` | Reads the MPRLS every 10 ms; tracks the baseline; produces sip and puff start/end events |
| `keyboard.c` | Maps characters to HID usages; queues them; sends press then release reports |
| `feedback.c` | Motor pulses, buzzer tone, LED |
| `display.c` | OLED layout; redraws one dirty line per loop pass |
| `settings.c` | Loads and saves settings in the last flash page |
| `i2c_bus.c` | Counts I2C failures and runs bus recovery |
| `usb_reconnect.c` | Pulls PA12 low for 10 ms at start-up |
| `log.c`, `sh1106.c`, `mprls.c`, `font5x7.c` | Already written for the bring-up test; reused as they are |

The C decoder is compiled straight from `decoder/c/morse_decoder.c` (CMake adds `../decoder/c` as a source and include path), so the firmware and the tests always use the same code.

## Main loop

```
app_loop():
  now = HAL_GetTick()
  for each event from inputs_poll(now)        -> handle_switch_event
  for each event from breath_poll(now)        -> handle_breath_event   (sip-and-puff mode only)
  n = md_tick(&dec, now, out, 4)              -> handle_outputs
  keyboard_poll()                             (sends the next queued report if USB is idle)
  feedback_poll(now)                          (ends motor pulses)
  display_poll(now)                           (redraws at most one line)
  settings_poll(now)                          (writes flash if dirty for 5 s)
```

No step blocks for more than 15 ms (an OLED line is about 12 ms at 100 kHz).

## Inputs (`inputs.c`)

```c
typedef enum { IN_BIG, IN_JACK_DIT, IN_JACK_DAH, IN_BTN_DIT, IN_BTN_DAH, IN_MODE, IN_COUNT } input_id_t;
typedef struct { input_id_t id; bool down; uint32_t at_ms; } input_event_t;
uint8_t inputs_poll(uint32_t now, input_event_t *events, uint8_t max);
```

Each input keeps `raw`, `stable` and `first_edge_ms`. When `raw` changes, `first_edge_ms = now`. When `raw` has differed from `stable` for 10 ms, `stable` flips and an event is produced with `at_ms = first_edge_ms`, so press lengths are measured edge to edge.

## Mode handling (`app.c`)

| Mode | Decoder mode | Key down / up comes from |
|---|---|---|
| One-button | `MD_SINGLE` | `IN_BIG` → `MD_KEY_SINGLE` |
| Two-switch | `MD_DUAL` | `IN_BTN_DIT` or `IN_JACK_DIT` → `MD_KEY_DIT`; `IN_BTN_DAH` or `IN_JACK_DAH` → `MD_KEY_DAH` |
| Sip-and-puff | `MD_DUAL` | sip → `MD_KEY_DIT`; puff → `MD_KEY_DAH` |

In two-switch mode, a key counts as down while either of its two inputs is down, so a jack switch and the matching button never double-press.

`IN_MODE`: on release, if it was held less than 1 s, go to the next mode, call `md_flush`, re-run `md_init` with the new mode, show the mode on the OLED, and mark settings dirty. If it has been held for 2 s, toggle sound once (and ignore the release).

### Element feedback

The C decoder does not report individual elements, so `app.c` works them out at key-up:

- single mode: press length `< 2 × md_unit_ms()` → dit, else dah
- dual mode: the key decides

It appends `.` or `-` to its own `pattern` string (shown on the OLED) and starts a 40 ms or 120 ms motor pulse. The pattern is cleared whenever the decoder outputs a character, a special code or unknown.

### Decoder outputs

| Output | Action |
|---|---|
| `MD_OUT_CHAR` | `keyboard_type_char(ch)` (lowercased), show as last letter |
| `MD_OUT_SPACE` | `keyboard_type_char(' ')` |
| `MD_OUT_BACKSPACE` | `keyboard_type_usage(0x2A)` |
| `MD_OUT_ENTER` | `keyboard_type_usage(0x28)` |
| `MD_OUT_ACCEPT` | `keyboard_type_usage(0x68)` (F13) |
| `MD_OUT_UNKNOWN` | show `?`, two 60 ms buzzes, type nothing |

Every output is logged, for example `[morse] char a` or `[morse] unknown`.

## USB keyboard (`keyboard.c`)

### Turning the CubeMX HID mouse into a keyboard

CubeMX's HID class is a mouse. Three changes make it a boot keyboard:

1. `Middlewares/ST/STM32_USB_Device_Library/Class/HID/Inc/usbd_hid.h`: `HID_EPIN_SIZE` 4 → 8, and the report descriptor size constant 74 → 63.
2. `.../Class/HID/Src/usbd_hid.c`: interface protocol 2 (mouse) → 1 (keyboard), and the report descriptor replaced with the standard 63-byte boot keyboard descriptor (modifier byte, reserved byte, 5 LED bits + 3 padding, 6 key codes).
3. `usbd_hid.c`: the HID descriptor's report length uses the new size.

CubeMX rewrites these files whenever code is generated, so the changes are applied by `firmware/tools/patch-hid-keyboard.ps1`, and `build.ps1` refuses to build (with a message saying to run the script) if the marker comment `DITDAH_KEYBOARD_PATCH` is missing from `usbd_hid.c`.

### Sending keys

```c
bool keyboard_type_char(char c);        /* false if the character can't be typed or the queue is full */
bool keyboard_type_usage(uint8_t usage);
void keyboard_poll(void);
bool keyboard_connected(void);
```

- A 32-entry ring buffer of `{modifier, usage}`.
- `keyboard_poll()` does nothing unless the device is configured and `USBD_HID` is idle. Then it sends the next step: a press report `{mod, 0, usage, 0, 0, 0, 0, 0}`, and on the following call a release report of all zeros.
- Character map (US layout):

| Characters | Usage | Shift |
|---|---|---|
| `a`–`z` | 0x04–0x1D | no |
| `1`–`9`, `0` | 0x1E–0x26, 0x27 | no |
| space | 0x2C | no |
| `-` | 0x2D | no |
| `'` | 0x34 | no |
| `,` | 0x36 | no |
| `.` | 0x37 | no |
| `/` | 0x38 | no |
| `?` | 0x38 | yes |
| `@` | 0x1F | yes |

## Breath (`breath.c`)

States: `BASELINE` (first second), `IDLE`, `PUFF`, `SIP`. Pressure is in tenths of a hPa from `mprls_poll`.

- `BASELINE`: average every reading for 1 s, then go to `IDLE`.
- `IDLE`: follow slow drift with a fixed-point average: `acc += p - acc / 1024`, `baseline = acc / 1024`. At 100 readings a second that is a time constant of about 10 s. If `p - baseline ≥ puff_threshold` → `PUFF` (event: dah down). If `baseline - p ≥ sip_threshold` → `SIP` (event: dit down).
- `PUFF`: when `p - baseline < 0.6 × puff_threshold` → `IDLE` (event: dah up). `SIP` mirrors it.
- Thresholds come from settings, default 100 (= 10.0 hPa). The baseline does not move during a sip or puff.
- If `mprls_poll` fails 10 times in a row, report `breath_ok() == false`; the display shows "No breath sensor".

## Feedback (`feedback.c`)

- `feedback_motor_pulse(ms)`: sets `MOTOR` high and remembers when to turn it off; `feedback_poll` turns it off. A new pulse extends the current one.
- `feedback_tone(bool on)`: `HAL_TIM_PWM_Start/Stop` on TIM1 CH1 (600 Hz from the .ioc). Ignored when sound is off.
- LED: on while a key is down, otherwise off.

## Display (`display.c`)

| Line | Content |
|---|---|
| 0 | `DitDah  12wpm  1-btn` (mode: `1-btn`, `2-sw`, `breath`) |
| 2 | `Code: .-.` |
| 3 | `Last: a` (or `?`) |
| 5 | last 21 characters typed |
| 7 | status: `USB ok` / `USB not connected`, `sound off`, `No breath sensor` |

Each line has a dirty flag. `display_poll` redraws at most one dirty line per pass, and at most every 30 ms.

## Settings (`settings.c`)

```c
typedef struct {
  uint32_t magic;          /* 'DDAH' */
  uint16_t version;        /* 1 */
  uint8_t  mode;           /* 0 one-button, 1 two-switch, 2 sip-and-puff */
  uint8_t  wpm;            /* 6..25, default 10 */
  uint8_t  adaptive;       /* default 1 */
  uint8_t  sound;          /* default 1 */
  uint8_t  motor;          /* default 1 */
  uint8_t  reserved;
  uint16_t puff_threshold; /* tenths of hPa, default 100 */
  uint16_t sip_threshold;  /* tenths of hPa, default 100 */
  uint32_t crc;            /* CRC-32 of everything above */
} settings_t;
```

- Stored at `0x0800FC00`. The linker script's FLASH length becomes 63K so code never reaches that page.
- `settings_load()` checks magic, version, CRC and ranges; anything wrong → defaults.
- `settings_mark_dirty()` + `settings_poll(now)`: erase and write the page 5 s after the last change, with `HAL_FLASH_Unlock`, `HAL_FLASHEx_Erase` (1 page) and half-word programming, then lock.

## I2C recovery (`i2c_bus.c`)

`i2c_bus_report(HAL_StatusTypeDef)` counts consecutive failures from the OLED and sensor drivers. On the third: de-init I2C1, drive PB6 as open-drain GPIO and pulse it 9 times at about 100 kHz with PB7 released, generate a STOP, then `MX_I2C1_Init()` again. Logged as `[i2c] bus recovered`.

## Error handling

| Situation | Behavior |
|---|---|
| USB not connected | Keys are dropped (not queued forever); status line says so |
| Keyboard queue full | New character dropped, `[usb] queue full` logged |
| OLED missing or failing | Everything else keeps working; logged once |
| Sensor missing or failing | Sip-and-puff mode shows "No breath sensor"; other modes unaffected |
| Bad settings in flash | Defaults |

## Testing strategy

- **Host tests (no hardware):** the C decoder already runs against the shared vectors (`decoder/c`, `make test`). Add a host test for the keyboard character map and the breath state machine by compiling `keyboard_map.c` and `breath_logic.c` (pure parts split out of the modules) with gcc in `firmware/test/`.
- **On the board:**
  1. Bring-up test: LED, I2C scan, OLED, pressure, inputs.
  2. USB: the big button types `e` on two computers.
  3. One-button: HELLO WORLD into Notepad.
  4. Two-switch: the same with the 30mm buttons and with jack switches.
  5. Sip-and-puff: SOS three times in a row.
  6. Special codes, settings survive unplugging, 10 USB replugs, 1 hour of use.
