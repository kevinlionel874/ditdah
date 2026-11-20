# Requirements Document

## Introduction

DitDah Studio is the web app that goes with the DitDah device. It turns Morse input into text, suggests the next word with AI so people press fewer keys, speaks the text aloud, and teaches Morse with short practice games.

It takes input two ways:

- **Spacebar mode:** the Studio decodes Morse itself. Holding the spacebar is the key. This is how judges and new users try DitDah without hardware. It uses the shared decoder in `decoder/`.
- **Device mode:** DitDah is plugged in. The device already decodes Morse and types ordinary keystrokes, so the Studio just receives letters, spaces, Backspace and Enter like any keyboard. F13 means "accept the suggestion".

The people using the Studio may have very limited movement, so every action must be reachable with few presses, and the page must work fully from the keyboard.

## Requirements

### Requirement 1: Type with the spacebar

**User Story:** As someone trying DitDah without the device, I want to hold the spacebar as a Morse key, so that I can type right away.

#### Acceptance Criteria

1. WHEN spacebar mode is on AND the spacebar goes down THEN the Studio SHALL call the decoder's `keyDown` with the current time, and SHALL call `keyUp` when it comes back up.
2. WHEN the browser repeats a held spacebar THEN the Studio SHALL ignore the repeated events.
3. WHILE spacebar mode is on THE Studio SHALL call the decoder's `tick` at least every 20 ms, so that letters and spaces appear without another press.
4. WHEN the decoder outputs a character THEN the Studio SHALL add it to the text; WHEN it outputs backspace THEN the Studio SHALL delete the last character.
5. WHILE a letter is being entered THE Studio SHALL show its dits and dahs live, for example `.-.`, next to the text.
6. WHEN the sidetone setting is on AND the spacebar is held THEN the Studio SHALL play a 600 Hz tone until it is released.
7. WHEN the decoder reports an unknown pattern THEN the Studio SHALL type nothing and SHALL show a short "not a letter" hint.

### Requirement 2: Use the DitDah device

**User Story:** As a DitDah owner, I want the Studio to work with what my device types, so that I get suggestions and speech while using my own switch.

#### Acceptance Criteria

1. WHEN device mode is on THEN the Studio SHALL treat the spacebar as an ordinary space and SHALL NOT run the Morse decoder.
2. WHEN the F13 key arrives THEN the Studio SHALL accept the first suggestion, in either mode.
3. WHEN F13 arrives while spacebar mode is on THEN the Studio SHALL switch to device mode and say so in a status message.
4. The Studio SHALL let the user switch modes from a clearly labeled control and SHALL remember the choice.

### Requirement 3: Speak the text

**User Story:** As someone who can't speak, I want my text read aloud, so that I can talk to the people around me.

#### Acceptance Criteria

1. WHEN the user presses the Speak button THEN the Studio SHALL read the current line aloud with the browser's speech synthesis.
2. WHEN Enter arrives (the `.-.-` code in spacebar mode, or the Enter key in device mode) THEN the Studio SHALL speak the current line and start a new line.
3. The Studio SHALL use the voice and speaking rate chosen in settings.
4. IF the browser has no speech synthesis THEN the Studio SHALL show a message saying speech is unavailable and SHALL keep working otherwise.

### Requirement 4: Suggest the next word

**User Story:** As someone for whom every press takes effort, I want the Studio to guess what I'm writing, so that I can finish words and phrases with one action.

#### Acceptance Criteria

1. WHEN the text changes THEN the Studio SHALL request suggestions, waiting until typing has paused for 300 ms.
2. The suggestion service SHALL return up to 3 completions of the current word and up to 2 short phrases that could follow the text.
3. The Studio SHALL show the suggestions under the text, with the first one marked as the one that will be accepted.
4. WHEN the user accepts (F13, Tab in spacebar mode, or the `..--` code) THEN the Studio SHALL replace the unfinished word with the first suggestion followed by a space.
5. WHEN the user clicks or focuses and activates any suggestion THEN the Studio SHALL use that one instead.
6. The Studio SHALL show how many Morse elements the accepted suggestions have saved in this session.

### Requirement 5: Suggestion service

**User Story:** As the team, we want suggestions that are fast, cheap and private, so that the demo never stalls and no one's words are kept.

#### Acceptance Criteria

1. The service SHALL be a server route at `POST /api/suggest` that takes `{ "text": string }` and returns `{ "words": string[], "phrases": string[], "source": "ai" | "fallback" }`.
2. The service SHALL call an OpenAI-compatible chat completions API using `AI_API_KEY`, `AI_BASE_URL` and `AI_MODEL` from environment variables, and SHALL never send the key to the browser.
3. IF the AI call fails, returns something that isn't the expected JSON, or takes longer than 1.5 seconds THEN the service SHALL answer from a built-in list of common words and phrases with `"source": "fallback"`.
4. IF the environment variables are missing THEN the service SHALL always use the fallback, so that the Studio runs locally with no key.
5. The service SHALL reject text longer than 500 characters with status 400.
6. The service SHALL NOT log or store the text it receives.
7. Every suggestion SHALL be plain text of at most 40 characters, and words SHALL contain only letters, digits and `'`.

### Requirement 6: Quick phrases

**User Story:** As someone who says the same things often, I want a board of ready phrases, so that I can say them with almost no presses.

#### Acceptance Criteria

1. The Studio SHALL show 8 numbered phrases, with sensible defaults such as "Yes", "No", "I need help" and "Thank you".
2. WHEN a line holds only a digit from 1 to 8 AND Enter arrives THEN the Studio SHALL speak that phrase instead of the digit and clear the line.
3. The user SHALL be able to edit the phrases in settings, and the Studio SHALL remember them.

### Requirement 7: Settings

**User Story:** As a user or caregiver, I want to adjust the Studio to the person using it, so that it fits their speed and senses.

#### Acceptance Criteria

1. The settings SHALL include: input mode, starting speed in WPM (6 to 25), adaptive speed on or off, sidetone on or off, voice, speaking rate, large text, high contrast, and the 8 phrases.
2. The Studio SHALL save settings in the browser's local storage and load them on the next visit.
3. IF local storage is unavailable or holds invalid data THEN the Studio SHALL use the defaults and keep working.
4. WHEN the starting speed changes THEN the Studio SHALL create the decoder with the matching unit (1200 / WPM ms).

### Requirement 8: Practice games

**User Story:** As someone new to Morse, I want short games, so that I can learn the code quickly.

#### Acceptance Criteria

1. The letter drill SHALL show a letter and its code, play its sound, and check whether the player enters it correctly, keeping a running accuracy and speed.
2. The word sprint SHALL show common words one at a time against a 60-second timer and report words per minute and accuracy at the end.
3. Both games SHALL take input from the spacebar and from the device, using the same mode setting as the Studio.
4. Both games SHALL use the shared `encode` and `MORSE` table from the decoder package.

### Requirement 9: How-to page

**User Story:** As a new user or caregiver, I want a short guide, so that I can start without asking anyone.

#### Acceptance Criteria

1. The how-to page SHALL explain the three ways to type (one button, two switches, sip and puff), how the Studio's two modes work, and the special codes for Backspace, Enter and Accept.
2. The page SHALL include a full Morse chart for A–Z and 0–9.
3. The page SHALL say plainly that DitDah is a student prototype, not a medical device.

### Requirement 10: Accessibility

**User Story:** As a person with limited movement or vision, I want the Studio to work with assistive technology, so that I can use it on my own.

#### Acceptance Criteria

1. Every control SHALL be reachable and usable with the keyboard alone, with a visible focus outline.
2. Newly typed text and status messages SHALL be announced to screen readers through a polite live region.
3. The large-text setting SHALL make the text area at least 32 px, and the high-contrast setting SHALL meet WCAG AA contrast.
4. WHEN the user prefers reduced motion THEN the Studio SHALL turn off animations.
5. The Studio's main page SHALL score 95 or higher in Lighthouse's accessibility audit.

### Requirement 11: Deployment

**User Story:** As the team, we want the Studio online, so that judges can try it from the Devpost link.

#### Acceptance Criteria

1. The Studio SHALL build with `npm run build` in `studio/` and deploy to Vercel with `studio` as the root directory.
2. The Studio SHALL work in current Chrome, Edge, Firefox and Safari on a laptop.
3. The deployed site SHALL work with no sign-in.
