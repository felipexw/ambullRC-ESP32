# Feature Specification: Onboard Sound Effects

**Feature Branch**: `[007-onboard-sound-effects]`

**Created**: 2026-09-15

**Status**: Draft

**Input**: User description: "Implement a A2DP audio intragion between the app and the esp32 (this project)" — corrected during design (see Clarifications) from Bluetooth audio streaming to fixed, onboard sound effects triggered over the existing control connection.

## Clarifications

### Session 2026-09-15

This feature was originally scoped as Bluetooth A2DP audio streaming (any phone audio routed to
the car's speaker). That approach was corrected mid-design to the following, simpler mechanism,
after the user reviewed the in-progress design:

- Q: Where does the sound-effect audio come from? → A: It is fixed and stored on the ESP32
  itself (a small, pre-programmed set of tones), never streamed or transmitted from the phone —
  there is no Bluetooth audio profile, no A2DP, and no general "play whatever the phone is
  playing" capability.
- Q: How is a sound effect triggered? → A: A new text command sent over the **existing** single
  Bluetooth control connection already used for drive/light commands — not a second pairing or
  connection type.
- Q: What happens if the horn is triggered again while it's already playing? → A: It is ignored.
  The currently playing horn finishes undisturbed; the user must trigger again after it finishes
  to hear it again. There is no restart or queueing.
- Q: When a drive command (steering/throttle) and sound-effect playback conflict, which is
  prioritized? → A: Drive commands must never be delayed, dropped, or degraded by sound-effect
  playback. The sound effect is not cut off either — it keeps playing to completion in the
  background; "priority" means drive-command responsiveness is never compromised, not that
  playback is interrupted.
- Q: How many sound effects are in scope? → A: A small fixed set — horn, engine, and siren —
  each triggered by its own command, using the same mechanism.
- Q: How is volume controlled? → A: No software volume control — fixed and adjusted physically
  via a hardware trim potentiometer, same as originally decided when this was still scoped as
  streaming.
- Q: How long does the horn sound last? → A: A fixed 1500ms, then it stops on its own.
- Q: What should the engine effect sound like? → A: A rumbling engine tone reminiscent of a V8
  (e.g. a Mustang), not a flat/plain beep. *(Superseded — the engine effect was later removed
  entirely; see the last entry below.)*
- Q: Should the siren effect remain in scope? → A: No — removed. The car is not an ambulance, so
  a siren doesn't fit; the fixed set is horn and engine only.
- Q: Should the engine sound remain a manual trigger command, or reflect the DC motor's actual
  state? → A: Fully automatic, no manual `ENGINE` trigger. The engine plays continuously: a low
  idle "stopped car" rumble whenever the DC motor isn't engaged, switching to a different,
  distinctly "running" tone whenever it is (UP or DOWN, either direction) — not a
  duration-limited one-shot triggered by a command. *(Superseded — see the last entry below.)*
- Q: How should the horn interact with the now-continuous engine tone? → A: The horn plays "on
  top of" the engine — both must be audible together, not one silencing the other. Given the
  hardware only has a single GPIO/LEDC channel driving one speaker, this is approximated by
  rapidly time-slicing between the horn and engine frequencies rather than adding a second
  speaker/output pin. *(Superseded — see the last entry below.)*
- Q: Should the engine sound stay? → A: No — removed entirely, after being tried on the device.
  Only the horn remains: the output is silent unless the horn is playing. This drops the automatic
  idle/running engine tone and the horn-over-engine layering that existed only to support it; the
  fixed set is now horn only.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Trigger the horn from the car while driving (Priority: P1)

A user driving the RC car with the companion app wants to trigger the horn, which plays from a
speaker on the car itself, so the sound follows the vehicle rather than staying next to the
driver.

**Why this priority**: This is the core value of the feature — without a triggered sound
actually reaching the car's speaker, there is nothing to build on. It must work before anything
else matters.

**Independent Test**: With the app already connected to the car (the existing control
connection), trigger the horn and confirm it is audible from the car's speaker.

**Acceptance Scenarios**:

1. **Given** the app is connected to the car, **When** the user triggers the horn (e.g., presses
   the horn button), **Then** the horn tone is heard from the car's speaker.
2. **Given** the horn finishes playing on its own, **When** the user triggers it again
   afterward, **Then** it plays normally.
3. **Given** the horn is currently playing through the car's speaker, **When** the user triggers
   it again before it finishes, **Then** the request is ignored — the currently playing horn
   continues uninterrupted to its natural end, with no restart or queued replay.
4. **Given** the horn is not playing, **When** the car is idle, driving, or freshly booted,
   **Then** the speaker is silent.

---

### User Story 2 - Driving stays fully responsive regardless of horn playback (Priority: P1)

A user driving the car wants steering, throttle, and other car controls to keep working exactly
as they do today — with no added lag, no missed commands — regardless of the horn.

**Why this priority**: This project's existing, safety-critical purpose is controlling a
physical vehicle. The horn is an enhancement layered onto the same connection driving commands
use; it must never be allowed to compromise driving responsiveness or safety.

**Independent Test**: While the horn is playing, send drive commands (steering/throttle) and
confirm the car responds with no added delay compared to when the horn isn't playing; confirm the
horn is not cut short by the drive commands either.

**Acceptance Scenarios**:

1. **Given** the horn is actively playing through the car's speaker, **When** the user sends a
   steering or throttle command from the app, **Then** the car responds with no added delay or
   missed command, exactly as it would with the horn silent.
2. **Given** the horn is actively playing, **When** the user sends drive commands, **Then** the
   horn is not stopped early because of them — it continues playing to its natural end in the
   background.
3. **Given** the car's control connection drops or times out (with or without the horn playing),
   **When** this occurs, **Then** the DC motor stops and the servo returns to neutral exactly as
   specified by existing safety behavior, unaffected by horn playback.

---

### Edge Cases

- What happens when the user triggers the horn while the app isn't connected to the car? The
  command can't reach the car at all (no different from any other command sent while
  disconnected) — no new behavior is needed here beyond what already exists.
- What happens if the horn is playing when the connection drops or times out? Existing safety
  behavior (motor stop, servo neutral) proceeds unaffected; the horn is a very short,
  self-contained clip and is not required to be silenced early — it simply finishes (or is cut
  off by the hardware losing power, same as any other in-progress physical behavior would be).
- What does the car sound like immediately after boot? Silent — nothing plays until the horn is
  triggered.
- What happens if the user triggers the horn again while it's already playing? Ignored — see User
  Story 1, Acceptance Scenario 3.
- What happens if an `ENGINE` (or `SIREN`) word is sent? Both were removed; they are treated as
  any other unrecognized line (malformed) and have no effect.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST let the user trigger the horn sound effect from the app, using the
  existing Bluetooth control connection already used for drive/light commands. No separate
  pairing or connection type is introduced.
- **FR-002**: Triggering the horn MUST play its fixed tone through a speaker connected to the car.
- **FR-003**: The horn's audio content MUST be fixed and stored on the ESP32 itself — never
  streamed, transmitted, or selected from the phone's own audio.
- **FR-004**: The horn MUST play for a fixed duration of 1500ms, then stop on its own.
- **FR-005**: While the horn is currently playing, triggering it again MUST be ignored — the
  currently playing horn continues uninterrupted to its natural end, with no restart or queued
  replay.
- **FR-006**: Once the horn finishes playing, the system MUST be immediately ready to play again
  on the next trigger. The speaker MUST be silent whenever the horn is not playing, including
  from power-on.
- **FR-007**: Sending a drive command (steering/throttle) MUST NOT be delayed, dropped, or
  degraded by horn playback.
- **FR-008**: The horn currently playing MUST NOT be interrupted or cut off by a drive command —
  the only thing that stops it early is its own natural end.
- **FR-009**: All existing safety behavior (motor stop and servo neutral on disconnect, malformed
  command, or timeout) MUST remain fully intact and unaffected by horn playback.
- **FR-010**: No software or app-side volume control is required — output level is fixed,
  adjusted physically via a hardware trim potentiometer in the speaker's analog signal path.

### Key Entities

- **Horn**: The only sound effect; a fixed-duration (1500ms), fixed onboard tone, manually
  triggered. Not user-customizable in this feature.
- **Horn playback state (busy)**: Whether the horn is currently playing — determines whether a
  new horn trigger is honored (FR-005/FR-006).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A triggered horn becomes audible from the car's speaker with no perceptible delay
  after the command is received (comparable to the existing drive/light command responsiveness
  already in place).
- **SC-002**: 100% of drive commands sent while the horn is playing produce the same car behavior
  (steering, throttle, safety stop) — with no added delay or missed commands — as when the horn
  is silent.
- **SC-003**: 100% of horn trigger attempts made while the horn is already playing are correctly
  ignored (no restart, no queued replay).
- **SC-004**: 0 instances of horn playback causing the car to violate its existing safety
  behavior (motor stop / servo neutral on disconnect, malformed command, or timeout).

## Assumptions

- The car has (or will have) a physical speaker connected to it that the firmware can drive
  directly; provisioning that speaker hardware is assumed to be already possible/in scope of this
  project's hardware layer, same as existing motor/servo/light wiring.
- "The app" refers to the existing companion Android app already used for driving control; the
  horn is triggered by a new word command added to the same existing Bluetooth protocol, not a
  new client or a new pairing.
- Exactly one sound effect is in scope: the horn (manually triggered). Siren was considered and
  removed — the car is not an ambulance. An automatic engine tone (idle/running, layered under the
  horn) was implemented and then removed after on-device testing.
- Volume is fixed and adjusted physically via a hardware trim potentiometer; there is no
  software/app-side volume control.
- The horn is excluded from the DC-motor/servo fail-safe scope (Constitution Principle V),
  mirroring the auxiliary-lights precedent (`006-four-led-lights-control`) — a disconnect,
  malformed command, or timeout does not need to explicitly silence the horn.
