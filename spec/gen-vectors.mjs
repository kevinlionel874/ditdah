// Generates the shared Morse decoder test cases used by both the TypeScript and C decoders.
// Run: node spec/gen-vectors.mjs   (writes spec/morse-vectors.json and spec/morse_vectors.h)
// Never edit the generated files by hand; change this script and run it again.
import { writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));

// Timing rules, in units (see .kiro/specs/decoder/design.md).
const RULES = { ditMaxUnits: 2, letterGapUnits: 2, wordGapUnits: 5 };

// Reference alphabet. Must match the decoder's table exactly.
export const MORSE = {
  A: ".-", B: "-...", C: "-.-.", D: "-..", E: ".", F: "..-.", G: "--.", H: "....", I: "..", J: ".---",
  K: "-.-", L: ".-..", M: "--", N: "-.", O: "---", P: ".--.", Q: "--.-", R: ".-.", S: "...", T: "-",
  U: "..-", V: "...-", W: ".--", X: "-..-", Y: "-.--", Z: "--..",
  0: "-----", 1: ".----", 2: "..---", 3: "...--", 4: "....-", 5: ".....", 6: "-....", 7: "--...", 8: "---..", 9: "----.",
  ".": ".-.-.-", ",": "--..--", "?": "..--..", "'": ".----.", "/": "-..-.", "-": "-....-", "@": ".--.-.",
};
export const SPECIAL = { "<BS>": "........", "<ENTER>": ".-.-", "<ACCEPT>": "..--" };

// Deterministic pseudo-random numbers so the jitter cases never change between runs.
function mulberry32(seed) {
  return () => {
    seed |= 0; seed = (seed + 0x6d2b79f5) | 0;
    let t = Math.imul(seed ^ (seed >>> 15), 1 | seed);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

// Turns a list of tokens (single characters, " " for a word gap, or "<BS>"-style specials)
// into signals. Each signal is one press: how long the key is held (on) and the silence after it (off).
function encode(tokens, unit, { jitter = 0, seed = 1, dual = false } = {}) {
  const rand = mulberry32(seed);
  const vary = (ms) => (jitter ? Math.round(ms * (1 + (rand() * 2 - 1) * jitter)) : ms);
  const signals = [];
  const letters = [];
  for (const tok of tokens) {
    if (tok === " ") { letters.push(" "); continue; }
    const pattern = SPECIAL[tok] ?? MORSE[tok];
    if (!pattern) throw new Error(`No Morse code for ${tok}`);
    letters.push(pattern);
  }
  letters.forEach((pattern, li) => {
    if (pattern === " ") return;
    const nextIsWord = letters[li + 1] === " ";
    const isLast = li === letters.length - 1;
    [...pattern].forEach((el, ei) => {
      const lastEl = ei === pattern.length - 1;
      const on = dual ? 50 : vary(el === "." ? unit : 3 * unit);
      let off;
      if (!lastEl) off = vary(unit);
      else if (isLast) off = vary(3 * unit);
      else if (nextIsWord) off = vary(7 * unit);
      else off = vary(3 * unit);
      const sig = { on, off };
      if (dual) sig.key = el === "." ? "dit" : "dah";
      signals.push(sig);
    });
  });
  return signals;
}

const tokensOf = (text) => [...text];
const cases = [];
const add = (c) => cases.push({ mode: "single", unitMs: 120, ...c });

// 1. Every character on its own, at 10 WPM.
for (const ch of Object.keys(MORSE)) {
  add({ id: `char-${ch}`, signals: encode([ch], 120), expected: [ch] });
}

// 2. Words and sentences.
for (const text of ["SOS", "HELLO WORLD", "I NEED WATER", "THE QUICK BROWN FOX", "CALL ME AT 5"]) {
  add({ id: `text-${text.replace(/ /g, "_")}`, signals: encode(tokensOf(text), 120), expected: tokensOf(text) });
}

// 3. Special codes.
add({ id: "special-backspace", signals: encode(["H", "I", "<BS>"], 120), expected: ["H", "I", "<BS>"] });
add({ id: "special-enter", signals: encode(["O", "K", "<ENTER>"], 120), expected: ["O", "K", "<ENTER>"] });
add({ id: "special-accept", signals: encode(["W", "A", "T", "<ACCEPT>"], 120), expected: ["W", "A", "T", "<ACCEPT>"] });

// 4. Unknown patterns are reported, never typed as a letter.
add({ id: "unknown-eight-dahs", signals: Array.from({ length: 8 }, (_, i) => ({ on: 360, off: i === 7 ? 360 : 120 })), expected: ["<?>"] });
add({ id: "unknown-nine-dits", signals: Array.from({ length: 9 }, (_, i) => ({ on: 120, off: i === 8 ? 360 : 120 })), expected: ["<?>"] });

// 5. Boundaries, just inside and just outside each threshold (unit 120 ms).
add({ id: "boundary-dit-1.9u", signals: [{ on: 228, off: 360 }], expected: ["E"] });
add({ id: "boundary-dah-2.1u", signals: [{ on: 252, off: 360 }], expected: ["T"] });
add({ id: "boundary-same-letter-1.9u", signals: [{ on: 120, off: 228 }, { on: 120, off: 360 }], expected: ["I"] });
add({ id: "boundary-new-letter-2.1u", signals: [{ on: 120, off: 252 }, { on: 120, off: 360 }], expected: ["E", "E"] });
add({ id: "boundary-same-word-4.9u", signals: [{ on: 120, off: 588 }, { on: 120, off: 360 }], expected: ["E", "E"] });
add({ id: "boundary-new-word-5.1u", signals: [{ on: 120, off: 612 }, { on: 120, off: 360 }], expected: ["E", " ", "E"] });

// 6. No doubled spaces, and no space before the first letter.
add({ id: "space-not-doubled", signals: [{ on: 120, off: 2000 }, { on: 120, off: 360 }], expected: ["E", " ", "E"] });

// 7. Other speeds, with the unit set to match.
add({ id: "speed-20wpm-PARIS", unitMs: 60, signals: encode(tokensOf("PARIS"), 60), expected: tokensOf("PARIS") });
add({ id: "speed-6wpm-PARIS", unitMs: 200, signals: encode(tokensOf("PARIS"), 200), expected: tokensOf("PARIS") });

// 8. Human timing: every duration off by up to 25%, fixed seeds.
for (const [text, seed] of [["HELLO WORLD", 7], ["SOS SOS", 11], ["I NEED WATER", 23], ["THE QUICK BROWN FOX", 42]]) {
  add({ id: `jitter25-${text.replace(/ /g, "_")}`, signals: encode(tokensOf(text), 120, { jitter: 0.25, seed }), expected: tokensOf(text) });
}

// 9. Two-switch mode: the key decides dit or dah, only gaps are timed.
for (const text of ["SOS", "HELLO WORLD"]) {
  cases.push({ id: `dual-${text.replace(/ /g, "_")}`, mode: "dual", unitMs: 120, signals: encode(tokensOf(text), 120, { dual: true }), expected: tokensOf(text) });
}
cases.push({ id: "dual-long-dit-is-still-dit", mode: "dual", unitMs: 120, signals: [{ key: "dit", on: 900, off: 360 }], expected: ["E"] });

const out = { version: 1, generatedBy: "spec/gen-vectors.mjs", rules: RULES, defaultUnitMs: 120, cases };
writeFileSync(join(here, "morse-vectors.json"), JSON.stringify(out, null, 2) + "\n");

// C header for the laptop test runner in decoder/c.
const cStr = (s) => '"' + s.replace(/\\/g, "\\\\").replace(/"/g, '\\"') + '"';
const keyNum = (k) => (k === "dit" ? 1 : k === "dah" ? 2 : 0);
let h = `/* Generated by spec/gen-vectors.mjs. Do not edit. */\n#ifndef MORSE_VECTORS_H\n#define MORSE_VECTORS_H\n#include <stdint.h>\n\n`;
h += `/* key: 0 = single-switch mode (duration decides), 1 = dit key, 2 = dah key */\n`;
h += `typedef struct { uint8_t key; uint16_t on_ms; uint16_t off_ms; } mv_signal_t;\n`;
h += `/* mode: 0 = single, 1 = dual. expected: tokens joined with '|', e.g. "H|I|<BS>" */\n`;
h += `typedef struct { const char *id; uint8_t mode; uint16_t unit_ms; const mv_signal_t *signals; uint16_t n_signals; const char *expected; } mv_case_t;\n\n`;
cases.forEach((c, i) => {
  h += `static const mv_signal_t mv_sig_${i}[] = {${c.signals.map((s) => `{${keyNum(s.key)},${s.on},${s.off}}`).join(",")}};\n`;
});
h += `\nstatic const mv_case_t MV_CASES[] = {\n`;
cases.forEach((c, i) => {
  h += `  {${cStr(c.id)}, ${c.mode === "dual" ? 1 : 0}, ${c.unitMs}, mv_sig_${i}, ${c.signals.length}, ${cStr(c.expected.join("|"))}},\n`;
});
h += `};\n#define MV_CASE_COUNT ${cases.length}\n\n#endif\n`;
writeFileSync(join(here, "morse_vectors.h"), h);

console.log(`Wrote ${cases.length} cases to spec/morse-vectors.json and spec/morse_vectors.h`);
