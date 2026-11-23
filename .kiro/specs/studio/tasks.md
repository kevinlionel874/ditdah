# Implementation Plan

Run these after the decoder spec is done (`cd decoder && npm test` passes). Commit after each task, and save a screenshot of anything interesting to `docs/kiro-moments/`.

- [x] 1. Create the Next.js app and link the decoder
  - In the repo root run `npx create-next-app@latest studio --ts --tailwind --eslint --app --no-src-dir --import-alias "@/*" --use-npm`
  - Add `"@ditdah/decoder": "file:../decoder"` to `studio/package.json` and `transpilePackages: ["@ditdah/decoder"]` to `next.config.ts`
  - Add Vitest (`npm i -D vitest`) with a `test` script, and a smoke test that imports `MORSE` from the decoder
  - `npm run build` and `npm test` both pass
  - _Requirements: 11.1_

- [ ] 2. Write the pure text functions
  - Create `studio/lib/text-edit.ts` with `applyOutput`, `currentLine`, `currentWord`, `acceptSuggestion`, `elementsSaved`, `newLine` exactly as in design.md
  - Create `studio/test/text-edit.test.ts` covering every row of the design table
  - _Requirements: 1.4, 4.4, 4.6_

- [ ] 3. Write settings and phrases
  - Create `studio/lib/settings.ts` (type, defaults, `loadSettings`, `saveSettings`, validation) and `studio/lib/phrases.ts` (`DEFAULT_PHRASES`, `phraseCommand`)
  - Tests in `studio/test/settings.test.ts` and `studio/test/phrases.test.ts`
  - _Requirements: 6.1, 6.2, 7.1, 7.2, 7.3_

- [ ] 4. Build the composer with spacebar Morse
  - Create `studio/lib/use-morse-key.ts`, `studio/lib/sidetone.ts` and `studio/components/Composer.tsx` as in design.md
  - The Studio page shows the text, the live pattern and a status line; holding the spacebar types letters
  - Ignore key repeat; release the key on window blur; tick every 15 ms
  - Manual check: type HELLO WORLD with the spacebar
  - _Requirements: 1.1–1.7_

- [ ] 5. Add speech and Enter
  - Create `studio/lib/speech.ts`; add a Speak button
  - Enter (`.-.-` or the Enter key) speaks the current line and starts a new one; a lone digit 1–8 speaks that phrase instead
  - Show a message and disable Speak when speech synthesis is missing
  - _Requirements: 3.1–3.4, 6.2_

- [ ] 6. Add device mode and the mode switch
  - Create `studio/components/ModeSwitch.tsx`; save the mode in settings
  - In device mode the spacebar types a normal space and the decoder is off; F13 accepts in both modes; F13 in spacebar mode switches to device mode with a status message
  - _Requirements: 2.1–2.4_

- [ ] 7. Deploy the basics
  - Push and check the Vercel deployment (root directory `studio`)
  - Manual check on the live URL: spacebar typing and Speak work
  - _Requirements: 11.1–11.3_

- [ ] 8. Build the suggestion service
  - Create `studio/lib/fallback-words.ts` (about 1,000 common English words, lowercase, most common first) and `studio/lib/suggest-core.ts` (`buildMessages`, `cleanResponse`, `fallbackSuggest`)
  - Create `studio/app/api/suggest/route.ts` as in design.md: 400 for bad input, fallback without env vars, 1.5 s timeout, no logging, `Cache-Control: no-store`
  - Tests in `studio/test/suggest-core.test.ts` and `studio/test/suggest-route.test.ts` with `fetch` mocked
  - _Requirements: 5.1–5.7_

- [ ] 9. Show and accept suggestions
  - Create `studio/lib/use-suggestions.ts` (300 ms debounce, cancel old requests, cache, browser-side fallback) and `studio/components/SuggestionBar.tsx`
  - Accept with F13, Tab (spacebar mode) or the `..--` code; any suggestion can also be chosen directly
  - Show the "elements saved" counter
  - Manual check: typing "i need w" offers a sensible word in under 1.5 s
  - _Requirements: 4.1–4.6_

- [ ] 10. Add the phrase board and settings panel
  - Create `studio/components/PhraseBoard.tsx` and `studio/components/SettingsPanel.tsx` with every setting from Requirement 7, including editable phrases and voice choice
  - Changing WPM or adaptive recreates the decoder
  - _Requirements: 6.1, 6.3, 7.1–7.4_

- [ ] 11. Build the practice games
  - Create `studio/app/practice/page.tsx` with the letter drill and the word sprint as in design.md
  - Both use the shared `MORSE` and `encode` and respect the input mode
  - _Requirements: 8.1–8.4_

- [ ] 12. Write the how-to page
  - Create `studio/app/how-to/page.tsx` and `studio/components/MorseChart.tsx` (A–Z and 0–9 from `MORSE`)
  - Explain the three ways to type, the two Studio modes, the special codes, and that DitDah is a student prototype, not a medical device
  - _Requirements: 9.1–9.3_

- [ ] 13. Accessibility pass
  - Skip link, live region (completed words and status only), visible focus, large text, high contrast, reduced motion
  - Run Lighthouse on the deployed Studio page and fix issues until accessibility is 95 or higher; save the score screenshot to `docs/kiro-moments/`
  - _Requirements: 10.1–10.5_
