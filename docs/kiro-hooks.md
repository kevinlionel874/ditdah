# Kiro hooks to set up

Hooks let Kiro run a check by itself whenever a file changes. Create them in Kiro's UI (Agent Hooks panel → **+**), so they are saved under `.kiro/hooks/` and committed with the repo. Judges look for them.

For each hook below, copy the name, trigger, file pattern and instructions exactly. After a hook runs for the first time, take a screenshot and save it in `docs/kiro-moments/`.

## 1. Decoder tests on save (winter)

| Field | Value |
|---|---|
| Name | Run decoder tests |
| Trigger | File saved |
| Files | `decoder/src/**/*.ts`, `decoder/test/**/*.ts` |

Instructions:

> Run `npm test` in the `decoder` folder. If everything passes, reply with one line saying how many tests passed. If anything fails, list each failing test with the expected and actual output, and suggest a fix in `decoder/src/`. Never change `spec/morse-vectors.json` or `spec/morse_vectors.h` to make a test pass.

## 2. Regenerate the shared test cases (winter)

| Field | Value |
|---|---|
| Name | Regenerate Morse vectors |
| Trigger | File saved |
| Files | `spec/gen-vectors.mjs` |

Instructions:

> Run `node spec/gen-vectors.mjs` from the repo root. Then run `npm test` in `decoder` and `make test` in `decoder/c`. Report the number of cases written and whether both decoders still pass. If one fails, explain whether the generator or the decoder is wrong, using the rules in `.kiro/specs/decoder/design.md`.

## 3. C decoder matches the TypeScript one (winter)

| Field | Value |
|---|---|
| Name | Run C decoder tests |
| Trigger | File saved |
| Files | `decoder/c/*.c`, `decoder/c/*.h` |

Instructions:

> Run `make test` in `decoder/c`. Report pass or fail. If a case fails, compare the C code with `decoder/src/morse-decoder.ts` and point out where they behave differently. The two must stay identical.

## 4. Firmware builds (Justin)

| Field | Value |
|---|---|
| Name | Check firmware build |
| Trigger | File saved |
| Files | `firmware/Core/Src/ditdah/**`, `firmware/Core/Inc/ditdah/**`, `firmware/CMakeLists.txt` |

Instructions:

> In PowerShell, run `.\build.ps1` in the `firmware` folder. Report any errors or warnings with file and line. Report the RAM and FLASH usage from the end of the output, and warn if RAM is over 16 KB or FLASH is over 63 KB. Check the changed code against `.kiro/steering/embedded.md` (no HAL_Delay in the main loop, no malloc, no float printf, I2C timeouts of 10 ms or less) and point out any rule it breaks.

## 5. Studio checks (winter, run by hand)

| Field | Value |
|---|---|
| Name | Studio checks |
| Trigger | Manual |

Instructions:

> In the `studio` folder, run `npm run lint`, `npm test` and `npm run build`. Summarize any failures. Then check that no file under `studio/` contains an API key or a value for `AI_API_KEY`, and that `/api/suggest` still falls back to the built-in word list when the AI variables are missing.

## 6. Secret guard (everyone)

| Field | Value |
|---|---|
| Name | No secrets in the repo |
| Trigger | File saved |
| Files | `**/*.ts`, `**/*.tsx`, `**/*.js`, `**/*.mjs`, `**/*.json`, `**/*.md`, `**/*.c`, `**/*.h` |

Instructions:

> Look at the file that was just saved. If it contains anything that looks like an API key, token or password (for example a long random string after `key`, `token` or `secret`, or a value for `AI_API_KEY`), say so immediately and tell me to move it to `studio/.env.local` or Vercel's environment settings. Otherwise say nothing.
