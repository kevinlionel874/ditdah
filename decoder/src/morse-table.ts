/**
 * morse-table.ts — Morse alphabet, lookup and encode helpers.
 *
 * The MORSE table must match spec/gen-vectors.mjs exactly.
 * Both the TypeScript and C decoders use the same alphabet.
 */
import type { Output } from "./morse-decoder.js";

/** International Morse: character → dot/dash pattern. */
export const MORSE: Record<string, string> = {
  // A–Z
  A: ".-",    B: "-...", C: "-.-.", D: "-..",  E: ".",    F: "..-.",
  G: "--.",   H: "....", I: "..",   J: ".---", K: "-.-",  L: ".-..",
  M: "--",    N: "-.",   O: "---",  P: ".--.", Q: "--.-", R: ".-.",
  S: "...",   T: "-",    U: "..-",  V: "...-", W: ".--",  X: "-..-",
  Y: "-.--",  Z: "--..",
  // 0–9
  0: "-----", 1: ".----", 2: "..---", 3: "...--", 4: "....-",
  5: ".....", 6: "-....", 7: "--...", 8: "---..", 9: "----.",
  // Punctuation
  ".": ".-.-.-", ",": "--..--", "?": "..--..", "'": ".----.",
  "/": "-..-.",  "-": "-....-", "@": ".--.-.",
};

/** Reverse map: pattern → character (built once at module load). */
const PATTERN_TO_CHAR: Record<string, string> = {};
for (const [ch, pat] of Object.entries(MORSE)) {
  PATTERN_TO_CHAR[pat] = ch;
}

/** Special codes (checked before the general alphabet). */
const SPECIAL: Record<string, Output> = {
  "........": { kind: "backspace" },
  ".-.-":     { kind: "enter" },
  "..--":     { kind: "accept" },
};

/**
 * Translate a dot/dash pattern to an Output.
 * Patterns longer than 8 elements are always unknown.
 * Patterns not in the alphabet or special codes are unknown.
 */
export function lookup(pattern: string): Output {
  if (pattern.length === 0 || pattern.length > 8) {
    return { kind: "unknown" };
  }
  const special = SPECIAL[pattern];
  if (special !== undefined) return special;
  const ch = PATTERN_TO_CHAR[pattern];
  if (ch !== undefined) return { kind: "char", value: ch };
  return { kind: "unknown" };
}

/**
 * Encode text into an array of Morse patterns, one per character.
 * Used by practice games. Throws on unknown characters.
 */
export function encode(text: string): string[] {
  return [...text].map((ch) => {
    const upper = ch.toUpperCase();
    const pat = MORSE[upper];
    if (pat === undefined) throw new Error(`No Morse pattern for '${ch}'`);
    return pat;
  });
}
