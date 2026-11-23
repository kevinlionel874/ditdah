# Implementation Plan

- [x] 1. Install the decoder package and check the test runner
  - Run `npm install` in `decoder/`
  - Run `npm test`; the shared-vector tests should fail only because `src/morse-decoder.ts` is missing
  - _Requirements: 8.1_

- [ ] 2. Write the alphabet and lookup
  - Create `decoder/src/morse-table.ts` with `MORSE` (character → pattern) matching `spec/gen-vectors.mjs` exactly
  - Add `lookup(pattern)` returning a `char`, `backspace`, `enter`, `accept` or `unknown` output, with patterns over 8 elements as `unknown`
  - Add `encode(text)` returning one pattern per character for the practice games
  - Add unit tests in `decoder/test/morse-table.test.ts`: every character round-trips through `encode` and `lookup`; the three special codes; a 9-element pattern is unknown
  - _Requirements: 3.1, 3.2, 3.3, 4.1, 4.2, 4.3_

- [x] 3. Write the single-switch decoder
  - Create `decoder/src/morse-decoder.ts` with `MorseDecoder` as specified in design.md: `keyDown`, `keyUp`, `tick`, `flush`, `unitMs`, `pattern`, `reset`
  - Implement the timing table, the word-in-progress flag and the "no doubled spaces" rule
  - Clamp the unit to 48–200 ms; treat negative durations as 0 and cap durations at 60 s
  - Export everything from `decoder/src/index.ts`, including `toToken`
  - _Requirements: 1.1–1.4, 2.1–2.4, 4.4, 6.1–6.5_

- [x] 4. Make every shared vector pass
  - Run `npm test` and fix the decoder until all 69 cases in `test/vectors.test.ts` pass
  - Do not change the vectors to make tests pass; if a vector looks wrong, raise it with the team
  - _Requirements: 8.1_

- [x] 5. Add two-switch mode
  - In dual mode the key passed to `keyDown`/`keyUp` decides dit or dah; press length is ignored
  - The `dual-*` vectors must pass
  - _Requirements: 5.1, 5.2, 5.3, 5.4_

- [ ] 6. Add property-based tests with fast-check
  - Create `decoder/test/properties.test.ts` with the four properties in design.md: round trip, 25% jitter, never crashes, adaptive convergence (the last one may be `it.todo` until task 7)
  - Use a small reference encoder inside the test file (text → signals at a given unit) so the tests don't depend on the code under test
  - Record any bug a property finds in `docs/kiro-moments/` with the shrunk counterexample
  - _Requirements: 9.1, 9.2_

- [ ] 7. Add adaptive speed
  - Implement the in-letter-silence average from design.md, clamped to 48–200 ms, only when `adaptive` is on
  - Turn on the adaptive convergence property and make it pass
  - _Requirements: 7.1, 7.2, 7.3, 7.4_

- [x] 8. Port the decoder to C
  - Create `decoder/c/morse_decoder.h` and `decoder/c/morse_decoder.c` with the C API in design.md
  - Plain C99, no dynamic memory, state of 64 bytes or less, no HAL includes
  - Same alphabet, rules and adaptive behavior as the TypeScript version
  - _Requirements: 8.1, 9.3_

- [x] 9. Test the C decoder against the shared vectors
  - Create `decoder/c/test_decoder.c` that runs every case in `spec/morse_vectors.h` with the runner from design.md and prints any mismatch
  - Create `decoder/c/Makefile` with `make test` (gcc, `-std=c99 -Wall -Wextra -Werror`), exiting non-zero on failure
  - Add a `static_assert` that `sizeof(md_decoder_t) <= 64`
  - _Requirements: 8.1, 9.3_
