// Runs every shared test case from spec/morse-vectors.json against the TypeScript decoder.
// The runner below must stay identical to the one in .kiro/specs/decoder/design.md and decoder/c/test_decoder.c.
import { readFileSync } from "node:fs";
import { describe, expect, it } from "vitest";
import { MorseDecoder, toToken, type Key, type Output } from "../src/index";

type Signal = { on: number; off: number; key?: "dit" | "dah" };
type VectorCase = { id: string; mode: "single" | "dual"; unitMs: number; signals: Signal[]; expected: string[] };

const vectors: { cases: VectorCase[] } = JSON.parse(
  readFileSync(new URL("../../spec/morse-vectors.json", import.meta.url), "utf8"),
);

function run(c: VectorCase): string[] {
  const decoder = new MorseDecoder({ unitMs: c.unitMs, mode: c.mode, adaptive: false });
  const outputs: Output[] = [];
  let t = 1000;
  for (const s of c.signals) {
    const key: Key = s.key ?? "single";
    outputs.push(...decoder.keyDown(t, key));
    t += s.on;
    outputs.push(...decoder.keyUp(t, key));
    t += s.off;
    outputs.push(...decoder.tick(t));
  }
  outputs.push(...decoder.flush());
  return outputs.map(toToken);
}

describe("shared Morse vectors", () => {
  it("has the expected number of cases", () => {
    expect(vectors.cases.length).toBeGreaterThan(60);
  });

  for (const c of vectors.cases) {
    it(c.id, () => {
      expect(run(c)).toEqual(c.expected);
    });
  }
});
