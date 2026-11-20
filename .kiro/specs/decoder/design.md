# Design Document: Morse decoder

## Overview

One small state machine, written twice: `decoder/src/morse-decoder.ts` for the Studio and `decoder/c/morse_decoder.c` for the Blue Pill. It receives key events with millisecond timestamps and returns outputs: a character, a space, Backspace, Enter, Accept or Unknown. It does not touch the keyboard, screen or network; callers decide what to do with each output.

## Timing model

`u` = the current unit in ms (default 120, clamped to 48–200).

| Event | Rule |
|---|---|
| Press shorter than `2u` | dit |
| Press `2u` or longer | dah |
| Silence shorter than `2u` | same letter continues |
| Silence reaches `2u` | end the letter, output it |
| Silence reaches `5u` | output a space, if a character has been output since the last space, Enter or Accept |

Ideal Morse timing is dit 1u, dah 3u, gap inside a letter 1u, between letters 3u, between words 7u. The thresholds sit between those values, so timing up to 25% off still decodes (dah at 75% is 2.25u ≥ 2u; a word gap at 75% is 5.25u ≥ 5u). At 30% off a word gap becomes 4.9u and would be read as a letter gap, so 25% is the promise.

## Outputs

| Kind | Meaning | Token in test vectors |
|---|---|---|
| `char` | a letter, digit or punctuation mark (uppercase) | the character itself, e.g. `A` |
| `space` | word gap | `" "` |
| `backspace` | pattern `........` | `<BS>` |
| `enter` | pattern `.-.-` | `<ENTER>` |
| `accept` | pattern `..--` | `<ACCEPT>` |
| `unknown` | any other pattern, or longer than 8 elements | `<?>` |

Word-in-progress flag: set after `char`; cleared after `space`, `enter` and `accept`; unchanged by `backspace` and `unknown`. A space is output only while the flag is set.

## Alphabet

International Morse for A–Z and 0–9, plus seven punctuation marks: `.` `.-.-.-`, `,` `--..--`, `?` `..--..`, `'` `.----.`, `/` `-..-.`, `-` `-....-`, `@` `.--.-.`. The exact table is the `MORSE` object in `spec/gen-vectors.mjs`; both decoders must match it.

## API

### TypeScript

```ts
export type Key = "single" | "dit" | "dah";
export type Mode = "single" | "dual";
export type Output =
  | { kind: "char"; value: string }
  | { kind: "space" | "backspace" | "enter" | "accept" | "unknown" };

export interface DecoderOptions {
  unitMs?: number;     // default 120, clamped to 48..200
  mode?: Mode;         // default "single"
  adaptive?: boolean;  // default false
}

export class MorseDecoder {
  constructor(options?: DecoderOptions);
  keyDown(nowMs: number, key?: Key): Output[];
  keyUp(nowMs: number, key?: Key): Output[];
  tick(nowMs: number): Output[];
  flush(): Output[];
  get unitMs(): number;
  get pattern(): string;   // dits and dahs of the letter in progress, e.g. ".-"
  reset(): void;
}

export const MORSE: Record<string, string>;      // character -> pattern
export function lookup(pattern: string): Output; // pattern -> output
export function encode(text: string): string[];  // for practice games: "SOS" -> ["...", "---", "..."]
export function toToken(o: Output): string;      // output -> test vector token
```

In single mode the `key` argument is ignored. In dual mode `keyDown(t, "dit")` or `"dah"` decides the element; press length is ignored.

### C

```c
typedef enum { MD_SINGLE = 0, MD_DUAL = 1 } md_mode_t;
typedef enum { MD_KEY_SINGLE = 0, MD_KEY_DIT = 1, MD_KEY_DAH = 2 } md_key_t;
typedef enum { MD_OUT_CHAR, MD_OUT_SPACE, MD_OUT_BACKSPACE, MD_OUT_ENTER, MD_OUT_ACCEPT, MD_OUT_UNKNOWN } md_kind_t;
typedef struct { md_kind_t kind; char ch; } md_out_t;

typedef struct { /* private: at most 64 bytes */ } md_decoder_t;

void     md_init(md_decoder_t *d, uint16_t unit_ms, md_mode_t mode, bool adaptive);
uint8_t  md_key_down(md_decoder_t *d, uint32_t now_ms, md_key_t key, md_out_t *out, uint8_t max_out);
uint8_t  md_key_up(md_decoder_t *d, uint32_t now_ms, md_key_t key, md_out_t *out, uint8_t max_out);
uint8_t  md_tick(md_decoder_t *d, uint32_t now_ms, md_out_t *out, uint8_t max_out);
uint8_t  md_flush(md_decoder_t *d, md_out_t *out, uint8_t max_out);
uint16_t md_unit_ms(const md_decoder_t *d);
```

Each function returns how many outputs it wrote (never more than `max_out`; callers pass at least 2). Elapsed time is computed as `(uint32_t)(now - then)`, which handles wrap-around.

## State machine

State: `unit`, `mode`, `adaptive`, `keyIsDown`, `downAt`, `upAt`, `pattern` (up to 9 elements; a 9th marks it too long), `letterOpen`, `wordInProgress`, `spaceSent`.

- **keyDown(t)**: if the key is already down, ignore. Otherwise first run the silence check for `t - upAt` (as in tick). Then, if adaptive and the silence was shorter than `2u`, update the unit. Record `downAt = t`.
- **keyUp(t)**: if the key isn't down, ignore. Classify (single: `t - downAt < 2u` is a dit; dual: by key). Append to `pattern`. Set `upAt = t`, `letterOpen = true`, `spaceSent = false`.
- **tick(t)** (silence check): if no key is down and `letterOpen` and silence ≥ `2u`, output `lookup(pattern)`, clear `pattern`, `letterOpen = false`. Then if silence ≥ `5u` and `wordInProgress` and not `spaceSent`, output a space, clear `wordInProgress`, set `spaceSent`.
- **flush()**: if `letterOpen`, output the letter (no space).

Zero or negative durations count as 0 ms. Durations above 60 s are capped at 60 s.

## Adaptive speed

Only the silences inside a letter (shorter than `2u`) are used, because in ideal Morse they are exactly 1 unit and they can't be confused with dits or dahs. Each such silence `s` updates `u ← u + (s − u) / 4`, then clamps to 48–200 ms. Starting from 120 ms this settles within about 8 letters at 20 WPM. At 6 WPM (200 ms) a silence inside a letter is still under the 240 ms letter threshold, so it pulls the unit up; until it has, a 600 ms letter gap can reach the 5u word threshold and add an extra space in the first word or two. Speeds slower than 6 WPM need the starting speed raised in settings.

## Test vectors

`spec/gen-vectors.mjs` writes 69 cases: every character, words, special codes, unknown patterns, each threshold just inside and outside, no doubled spaces, 20 and 6 WPM, 25% random jitter with fixed seeds, and two-switch cases.

Each case has `mode`, `unitMs`, `signals` (each `{on, off, key?}`) and `expected` tokens. The runner, identical in both languages:

```
t = 1000
for each signal:
  collect keyDown(t, key)
  t += on;  collect keyUp(t, key)
  t += off; collect tick(t)
collect flush()
compare tokens with expected
```

## Property-based tests (fast-check)

1. **Round trip**: any text of 1–30 tokens from A–Z, 0–9 and single spaces (no leading, trailing or doubled spaces), encoded at any unit from 48 to 200 ms and decoded with that unit, gives the same tokens.
2. **Jitter**: the same, with every duration multiplied by a factor in [0.75, 1.25].
3. **Never crashes**: any sequence of up to 200 random events (key down or up with any key, tick, flush) at any non-decreasing timestamps never throws and every output has a valid kind.
4. **Adaptive convergence**: for any true unit from 60 to 190 ms, "PARIS PARIS PARIS" decoded with adaptive on, starting at 120 ms, ends with "PARIS" and a unit within 15% of the true unit.

## Error handling

The decoder never throws on input. Invalid options are clamped. Unknown patterns become `unknown` outputs, which the Studio may show as "?" and the device ignores.

## Testing strategy

- Vitest runs `decoder/test/vectors.test.ts` (shared cases) and `decoder/test/properties.test.ts` (fast-check).
- `decoder/c/test_decoder.c` runs the same cases from `spec/morse_vectors.h`; `make test` exits non-zero on any failure.
- A Kiro hook runs the TypeScript tests whenever a file in `decoder/` is saved.
