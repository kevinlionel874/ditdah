/**
 * morse-decoder.ts — TypeScript Morse decoder for DitDah.
 *
 * Implements the same state machine as decoder/c/morse_decoder.c.
 * See .kiro/specs/decoder/design.md for the timing rules.
 */
import { lookup } from "./morse-table.js";

export type Key = "single" | "dit" | "dah";
export type Mode = "single" | "dual";
export type Output =
  | { kind: "char"; value: string }
  | { kind: "space" | "backspace" | "enter" | "accept" | "unknown" };

export interface DecoderOptions {
  /** Unit length in ms. Default 120, clamped to 48–200. */
  unitMs?: number;
  /** Default "single". */
  mode?: Mode;
  /** Default false. */
  adaptive?: boolean;
}

/** Maximum duration accepted (durations above this are capped). */
const MAX_DURATION_MS = 60_000;
/** Minimum and maximum unit values. */
const UNIT_MIN = 48;
const UNIT_MAX = 200;
const UNIT_DEFAULT = 120;

function clampUnit(u: number): number {
  return Math.max(UNIT_MIN, Math.min(UNIT_MAX, u));
}

function clampDuration(d: number): number {
  if (d < 0) return 0;
  if (d > MAX_DURATION_MS) return MAX_DURATION_MS;
  return d;
}

/** Update the word-in-progress flag after emitting an output. */
function updateWip(wip: boolean, kind: Output["kind"]): boolean {
  switch (kind) {
    case "char":    return true;
    case "space":
    case "enter":
    case "accept":  return false;
    default:        return wip; // backspace, unknown — unchanged
  }
}

export class MorseDecoder {
  private _unit: number;
  private readonly _mode: Mode;
  private readonly _adaptive: boolean;

  // Initial option values kept for reset()
  private readonly _initUnit: number;

  private _keyIsDown = false;
  private _downAt = 0;
  private _upAt = 0;
  private _pattern = "";       // dots and dashes of the letter in progress
  private _letterOpen = false;
  private _wordInProgress = false;
  private _spaceSent = false;

  constructor(options?: DecoderOptions) {
    const u = options?.unitMs ?? UNIT_DEFAULT;
    this._initUnit = clampUnit(u);
    this._unit = this._initUnit;
    this._mode = options?.mode ?? "single";
    this._adaptive = options?.adaptive ?? false;
  }

  get unitMs(): number { return this._unit; }

  /** Dits and dahs accumulated for the letter in progress, e.g. ".-". */
  get pattern(): string { return this._pattern; }

  /** Reset every piece of state to its initial value (keeps original options). */
  reset(): void {
    this._unit = this._initUnit;
    this._keyIsDown = false;
    this._downAt = 0;
    this._upAt = 0;
    this._pattern = "";
    this._letterOpen = false;
    this._wordInProgress = false;
    this._spaceSent = false;
  }

  /**
   * Run the silence check: may close the in-progress letter and/or emit
   * a word-space. Called internally by tick() and keyDown().
   */
  private _silenceCheck(nowMs: number): Output[] {
    const outputs: Output[] = [];
    const silence = clampDuration(nowMs - this._upAt);

    if (this._letterOpen && silence >= 2 * this._unit) {
      const out = lookup(this._pattern);
      outputs.push(out);
      this._wordInProgress = updateWip(this._wordInProgress, out.kind);
      this._pattern = "";
      this._letterOpen = false;
    }

    if (
      !this._keyIsDown &&
      this._wordInProgress &&
      !this._spaceSent &&
      silence >= 5 * this._unit
    ) {
      outputs.push({ kind: "space" });
      this._wordInProgress = false;
      this._spaceSent = true;
    }

    return outputs;
  }

  /**
   * Signal key-down at timestamp nowMs.
   * Runs the silence check first; records downAt.
   */
  keyDown(nowMs: number, key?: Key): Output[] {
    if (this._keyIsDown) return [];

    // Adaptive speed update: only in-letter silences (< 2u) update the unit.
    if (this._adaptive && this._letterOpen) {
      const silence = clampDuration(nowMs - this._upAt);
      if (silence < 2 * this._unit) {
        this._unit = clampUnit(this._unit + (silence - this._unit) / 4);
      }
    }

    const outputs = this._silenceCheck(nowMs);
    this._downAt = nowMs;
    this._keyIsDown = true;
    return outputs;
  }

  /**
   * Signal key-up at timestamp nowMs.
   * Classifies the press as dit or dah and appends it to the pattern.
   */
  keyUp(nowMs: number, key?: Key): Output[] {
    if (!this._keyIsDown) return [];

    let element: "." | "-";
    if (this._mode === "dual") {
      // In dual mode the key argument decides, press length is irrelevant.
      element = key === "dah" ? "-" : ".";
    } else {
      const press = clampDuration(nowMs - this._downAt);
      element = press < 2 * this._unit ? "." : "-";
    }

    // Cap at 9 characters — the 9th is a sentinel for "too long".
    if (this._pattern.length < 8) {
      this._pattern += element;
    } else {
      // Mark overflow (9 chars → lookup will return unknown)
      this._pattern = this._pattern.slice(0, 8) + element;
    }

    this._upAt = nowMs;
    this._letterOpen = true;
    this._spaceSent = false;
    this._keyIsDown = false;

    return [];
  }

  /**
   * Poll during silence; may output a completed letter or word-space.
   */
  tick(nowMs: number): Output[] {
    if (this._keyIsDown) return [];
    return this._silenceCheck(nowMs);
  }

  /**
   * Flush the in-progress letter (if any) without waiting for a silence.
   * Never emits a space.
   */
  flush(): Output[] {
    if (!this._letterOpen) return [];
    const out = lookup(this._pattern);
    this._wordInProgress = updateWip(this._wordInProgress, out.kind);
    this._pattern = "";
    this._letterOpen = false;
    return [out];
  }
}

/**
 * Convert an Output to its test-vector token.
 * char → its value; space → " "; backspace → "<BS>";
 * enter → "<ENTER>"; accept → "<ACCEPT>"; unknown → "<?>"
 */
export function toToken(o: Output): string {
  switch (o.kind) {
    case "char":      return o.value;
    case "space":     return " ";
    case "backspace": return "<BS>";
    case "enter":     return "<ENTER>";
    case "accept":    return "<ACCEPT>";
    case "unknown":   return "<?>";
  }
}
