// Smoke test: the Studio uses the shared decoder package (../decoder), never a copy of it.
import { describe, expect, it } from "vitest";
import { MORSE, MorseDecoder, toToken } from "@ditdah/decoder";

describe("shared decoder", () => {
  it("has the alphabet", () => {
    expect(MORSE.E).toBe(".");
    expect(MORSE.S).toBe("...");
  });

  it("decodes one dit as E", () => {
    const decoder = new MorseDecoder({ unitMs: 120 });
    decoder.keyDown(1000);
    decoder.keyUp(1100);
    expect(decoder.flush().map(toToken)).toEqual(["E"]);
  });
});
