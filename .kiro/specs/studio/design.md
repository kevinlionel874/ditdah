# Design Document

## Overview

DitDah Studio is a Next.js (App Router) app in `studio/`, written in strict TypeScript with Tailwind CSS, deployed to Vercel. It has three pages and one server route:

| Path | What it is |
|---|---|
| `/` | The Studio: text, live Morse, suggestions, speech, phrase board, settings |
| `/practice` | Letter drill and word sprint |
| `/how-to` | Guide and Morse chart |
| `POST /api/suggest` | Word and phrase suggestions (AI with a built-in fallback) |

All the logic that can be pure is pure and lives in `studio/lib/`, so it is unit-tested without a browser. React components stay thin.

## Using the shared decoder

`studio/package.json` depends on the decoder package by path:

```json
"dependencies": { "@ditdah/decoder": "file:../decoder" }
```

`next.config.ts` sets `transpilePackages: ["@ditdah/decoder"]`, because the package ships TypeScript source. The Studio imports `MorseDecoder`, `MORSE`, `encode`, `lookup` and `toToken` from it and never copies decoder logic. On Vercel, the project's root directory is `studio` with "Include files outside the root directory" left on, so `../decoder` is available at build time.

## Folder layout

```
studio/
  app/
    layout.tsx            fonts, theme classes, skip link, live region
    page.tsx              Studio page
    practice/page.tsx
    how-to/page.tsx
    api/suggest/route.ts
  components/
    Composer.tsx          text area, live pattern, status line
    SuggestionBar.tsx
    PhraseBoard.tsx
    SettingsPanel.tsx
    ModeSwitch.tsx
    MorseChart.tsx
  lib/
    text-edit.ts          pure text operations (below)
    settings.ts           defaults, load, save, validate
    phrases.ts            default phrases, phrase command parsing
    suggest-core.ts       prompt building, response cleaning, fallback suggester
    fallback-words.ts     about 1,000 common English words, most common first
    use-morse-key.ts      React hook: spacebar → decoder
    use-suggestions.ts    React hook: debounce, fetch, cancel
    sidetone.ts           Web Audio 600 Hz tone
    speech.ts             speechSynthesis wrapper
  test/                   Vitest unit tests
```

## Text model

The text is one string. Lines are separated by `\n`. The **current line** is the text after the last `\n`; the **current word** is the text after the last space on the current line.

All text is stored in lowercase. Decoder output in spacebar mode is lowercased before it is added; the device sends lowercase keystrokes anyway. Suggestions are inserted in lowercase.

`lib/text-edit.ts` (pure, fully tested):

| Function | Behavior |
|---|---|
| `applyOutput(text, output)` | `char` → append lowercase char; `space` → append `" "` unless the text is empty or already ends in a space or newline; `backspace` → remove the last character; `enter`, `accept`, `unknown` → text unchanged (handled by the caller) |
| `currentLine(text)` | text after the last `\n` |
| `currentWord(text)` | text after the last space on the current line |
| `acceptSuggestion(text, suggestion)` | replace the current word with `suggestion + " "`; if the suggestion is a phrase, append it after the current line's last complete word |
| `elementsSaved(prefix, word)` | sum of Morse element counts (via `encode`) for the letters of `word` after `prefix` |
| `newLine(text)` | append `\n` |

## Spacebar input (`use-morse-key.ts`)

- Creates one `MorseDecoder` with `unitMs = 1200 / settings.wpm`, `mode: "single"` and `adaptive: settings.adaptive`. It is recreated when those settings change.
- Listens for `keydown` and `keyup` on the composer. For `event.code === "Space"`: `preventDefault()`, ignore `event.repeat`, then call `keyDown(performance.now())` or `keyUp(performance.now())`.
- Runs `decoder.tick(performance.now())` every 15 ms with `setInterval` while spacebar mode is on.
- On window `blur` while the key is down, calls `keyUp`, so the key never sticks.
- Every output goes to a single handler in the page:
  - `char`, `space`, `backspace` → `applyOutput`
  - `enter` → speak the current line (or run a phrase command), then `newLine`
  - `accept` → accept the first suggestion
  - `unknown` → status message "Not a letter, try again"
- Exposes `pattern` (the dits and dahs of the letter in progress) for the live display.
- Starts and stops the sidetone on key down and up when the setting is on.

## Device input

In device mode the composer is an ordinary text area. Its `onKeyDown` handles three keys:

| Key | Action |
|---|---|
| `F13` | `preventDefault()`, accept the first suggestion |
| `Enter` | `preventDefault()`, speak the current line (or run a phrase command), then `newLine` |
| everything else | default browser behavior |

In spacebar mode, F13 also accepts and switches the mode to device (Requirement 2.3). Tab accepts the first suggestion only in spacebar mode; in device mode Tab keeps its normal focus behavior.

## Suggestions

### Client (`use-suggestions.ts`)

- Waits until the text has not changed for 300 ms, then sends `POST /api/suggest` with the last 300 characters of the text.
- Cancels the previous request with an `AbortController` when a new one starts.
- Keeps the last result for the same text, so going back and forth costs nothing.
- On any error, shows fallback suggestions computed in the browser with the same `suggest-core.ts` functions.

### Server (`app/api/suggest/route.ts`)

1. Parse the body. If `text` is missing, not a string, or longer than 500 characters → `400`.
2. If `AI_API_KEY`, `AI_BASE_URL` or `AI_MODEL` is missing → return the fallback.
3. `fetch(`${AI_BASE_URL}/chat/completions`)` with `Authorization: Bearer ${AI_API_KEY}`, `AbortSignal.timeout(1500)`, `temperature: 0.3`, `max_tokens: 120`, and the prompt from `buildMessages(text)`.
4. `cleanResponse(raw, prefix)` extracts the first `{...}` block, parses it, and keeps only valid items:
   - words: match `/^[a-z0-9']{1,40}$/` after lowercasing, start with the current word, no duplicates, at most 3
   - phrases: plain text, newlines removed, 1 to 40 characters, at most 2
5. If fewer than 3 words survive, top up from the fallback. If nothing usable came back, or anything threw, return the fallback with `source: "fallback"`.
6. Never log the text. Set `Cache-Control: no-store`.

### Prompt (`buildMessages`)

System message: "You help a person who types very slowly using Morse code (an AAC user). Given their text, return JSON only: {\"words\": [3 likely completions of the last, unfinished word], \"phrases\": [2 short phrases, under 6 words, that could come next]}. Lowercase. No explanations." User message: the text.

### Fallback (`fallbackSuggest(text)`)

- Words: the first 3 entries of `fallback-words.ts` that start with the current word. With no current word, the 3 most common words.
- Phrases: two defaults chosen by simple rules: after "i need" → "help please", "water please"; after "thank" → "thank you"; otherwise "yes please" and "no thank you".

## Speech (`speech.ts`)

`speak(text, { voiceName, rate })` cancels anything already speaking, creates a `SpeechSynthesisUtterance`, picks the voice by name if it exists, and speaks. `isSpeechAvailable()` checks `window.speechSynthesis`. Voices load asynchronously, so the settings panel listens for `voiceschanged`.

## Phrase board (`phrases.ts`)

`DEFAULT_PHRASES`: "Yes", "No", "I need help", "Thank you", "Please wait", "I'm in pain", "Please call someone", "I'm okay".

`phraseCommand(line)` returns the phrase index when the line, trimmed, is exactly one digit from 1 to 8, otherwise `null`. On Enter, if it isn't `null`, the Studio speaks that phrase and clears the line instead of speaking the digit.

## Settings (`settings.ts`)

```ts
type Settings = {
  version: 1;
  inputMode: "spacebar" | "device";
  wpm: number;            // 6..25, default 10
  adaptive: boolean;      // default true
  sidetone: boolean;      // default true
  voiceName: string | null;
  speechRate: number;     // 0.5..2, default 1
  largeText: boolean;     // default false
  highContrast: boolean;  // default false
  phrases: string[];      // exactly 8, each 1..60 chars
};
```

`loadSettings()` reads `localStorage["ditdah-settings"]` inside `try/catch`, validates every field, and falls back to the default for any field that is missing or invalid. `saveSettings()` writes inside `try/catch` and ignores failures.

## Practice games (`/practice`)

Both games use the same input as the Studio (spacebar through `use-morse-key`, or device keystrokes), chosen by `settings.inputMode`.

- **Letter drill:** picks a random letter (starting with E, T, A, N, I, M and widening as accuracy passes 80%), shows the letter and its pattern from `MORSE`, plays the pattern with the sidetone, then checks the next decoded letter. Shows accuracy and average time per letter.
- **Word sprint:** 60-second timer, shows common words from `fallback-words.ts` (3 to 5 letters), advances when the typed word matches. At the end shows WPM (`correct characters / 5 / minutes`) and accuracy.

## Accessibility

- A skip link to the composer; all controls are native buttons, inputs and links.
- One polite `aria-live` region in `layout.tsx` announces each completed word, accepted suggestion and status message. Single letters are not announced, to keep it quiet.
- `largeText` adds a class that sets the composer to 32 px; `highContrast` swaps the color tokens to a palette that meets WCAG AA.
- Animations are wrapped in `@media (prefers-reduced-motion: no-preference)`.
- Visible focus outline on every control (3 px, offset).

## Error handling

| Situation | Behavior |
|---|---|
| AI times out, errors or returns junk | Fallback suggestions, `source: "fallback"` |
| No AI env vars (local dev) | Fallback suggestions, no error shown |
| `/api/suggest` unreachable | Browser-side fallback |
| No speech synthesis | Speak button disabled with a message |
| No Web Audio | Sidetone setting hidden |
| Local storage unavailable | Defaults, settings last for the session |
| Window loses focus while space is held | `keyUp` is sent |

## Testing strategy

Vitest, in `studio/test/`:

- `text-edit.test.ts`: every function in the table above, including no doubled spaces, backspace on empty text, accepting with and without a current word, `elementsSaved("hel", "hello")` = 7 (L is `.-..`, O is `---`)
- `settings.test.ts`: defaults, invalid JSON, out-of-range values, wrong phrase count
- `phrases.test.ts`: `phraseCommand` for "3", " 8 ", "9", "12", "a"
- `suggest-core.test.ts`: `cleanResponse` with valid JSON, JSON inside extra text, bad words, too-long phrases, duplicates; `fallbackSuggest` with and without a prefix
- `suggest-route.test.ts`: the route handler with `fetch` mocked: success, timeout, bad JSON, missing env vars, 501-character text → 400

Manual checks before deploy: type HELLO WORLD with the spacebar; accept with Tab and with the `..--` code; speak with `.-.-`; phrase "3" + Enter; switch to device mode and type normally; Lighthouse accessibility ≥ 95.
