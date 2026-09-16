---
description: "Task list for Onboard Sound Effects"
---

# Tasks: Onboard Sound Effects

**Input**: Design documents from `/specs/007-onboard-sound-effects/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md,
contracts/tone-command-protocol.md, contracts/tone-output-hardware-contract.md, quickstart.md

**Tests**: Included and REQUIRED — Constitution Principle II (Test-First) is non-negotiable for
this project: unit tests for `parseHornCommand()`, `ToneControl`, and `motorEngaged()`, plus
integration tests for both the horn trigger path and the automatic drive-command-to-engine-state
path, are mandatory. `PwmToneOutput` itself is a thin ESP32-only wrapper around the LEDC
peripheral (same shape as `PwmSteeringServo`/`GpioMotorDriver`/`GpioLightsOutput`) and is
validated on-device via `quickstart.md` instead of a host unit test — established precedent, not a
gap (see plan.md's Constitution Check).

**Scope reminder** (from plan.md, as revised by T020 below — task bodies below this line are left
unedited as historical record and predate that revision): this feature supersedes an earlier
"A2DP Audio Streaming" draft for the same slot (see spec.md Clarifications). It adds one one-shot
trigger word (`HORN`) to the existing drive/light-command vocabulary, a new `ToneControl` (Control
layer), a `motorEngaged(Direction)` helper (Control layer), and a new
`IToneOutput`/`PwmToneOutput` (Hardware layer) that synthesizes fixed waveforms locally on a
single GPIO (26) via LEDC — no Bluetooth audio profile, pairing, or third-party dependency. The
engine tone is fully automatic (no wire command), driven by `main.cpp` reading the already-decided
`Direction` every time a drive command is handled. No changes to `direction_control`,
`lights_control`, `ITransport`, or any existing `Hardware` implementation. (A third word, `SIREN`,
was implemented then removed — the car is not an ambulance; `ENGINE` was also removed as a manual
trigger once the engine sound became automatic.)

**Organization**: Tasks are grouped by user story (spec.md) to enable independent implementation
and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2)

## Path Conventions

Single embedded project per plan.md: new files live in `src/protocol/`, `src/control/`,
`src/hardware/`, and `test/test_native/` (unit tests + `fakes/`) and one new integration test
file; `src/config.h`, `src/main.cpp`, `platformio.ini`, and `README.md` are extended, not
replaced.

---

## Phase 1: Setup

**Purpose**: Remove the superseded A2DP-design artifacts and repurpose the config values that
carry over

- [X] T001 Remove the superseded A2DP-design code: delete `src/hardware/i_audio_connection.h`,
      `test/test_native/fakes/fake_audio_connection.h`, and
      `test/test_native/test_audio_connection_logging_flow.cpp`; remove their `extern`/
      `RUN_TEST` entries from `test/test_native/test_main.cpp`; remove the
      `pschatzmann/ESP32-A2DP` line from `platformio.ini`'s `[env:esp32dev]` `lib_deps` (spec
      Clarifications, plan.md's superseded-design note); run `pio test -e native` afterward to
      confirm the suite still builds/passes with those files gone — confirmed: 89/89 passing
      (down from 92, correctly reflecting the 3 removed audio-connection tests)
- [X] T002 [P] Replace the A2DP I2S pin config in `src/config.h` with `kToneOutputPin` = 26 (a
      single GPIO driving a speaker/buzzer via LEDC tone generation, per explicit instruction)
      plus per-effect duration/frequency constants: `kHornDurationMs`=1500, `kEngineDurationMs`,
      `kSirenDurationMs`, `kEngineBaseFreqHz`/`kEngineWobbleFreqHz` (V8-idle-mimicking rumble,
      research.md §6), `kHornFreqHz`, `kSirenMinFreqHz`/`kSirenMaxFreqHz` — done ahead of
      schedule during design correction

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Shared types/interfaces/test doubles every user story depends on

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T003 [P] Define `enum class SoundEffectId { Horn, Engine, Siren };`, `struct ToneCommand {
      SoundEffectId effect; };`, and declare `ParseResult parseToneCommand(const std::string&
      line, ToneCommand& out);` in NEW `src/protocol/tone_command_parser.h` (data-model.md)
- [X] T004 [P] Define the `IToneOutput` interface (`virtual void play(SoundEffectId effect) = 0;
      virtual bool busy() = 0; virtual void tick(unsigned long nowMs) = 0;`) in NEW
      `src/hardware/i_tone_output.h`
- [X] T005 Implement `FakeToneOutput` test double (records every `play()` call's `SoundEffectId`
      in order; a `setBusy(bool)` setter and `busy()` getter; `tick()` a no-op) in NEW
      `test/test_native/fakes/fake_tone_output.h` (depends on T004)

**Checkpoint**: Foundation ready — user story implementation can now begin

---

## Phase 3: User Story 1 - Trigger a sound effect from the car while driving (Priority: P1) 🎯 MVP

**Goal**: `HORN`/`ENGINE` commands play the corresponding onboard sound effect through
the car's speaker; a retrigger (same or different effect) while one is already playing is
ignored; the system is ready again immediately once playback finishes. (Originally implemented
with a third command, `SIREN` — removed post-completion, see Notes.)

**Independent Test**: Per quickstart.md steps 3–6: send `HORN` and confirm it plays; send `HORN`
again while it's still playing and confirm nothing changes; wait for it to finish, then confirm
`ENGINE` plays normally afterward.

### Tests for User Story 1 ⚠️

> Write these tests FIRST, ensure they FAIL before implementation

- [X] T006 [P] [US1] Unit tests for `parseToneCommand()`: `Ok` with the correct `effect` for
      `HORN`/`ENGINE`/`SIREN`, case-insensitively; `Malformed` for every existing drive word
      (`UP`, `LEFT`, ...), every existing light word (`LIGHT1ON`, ...), a numeric pair (`"0,0"`),
      and garbage (so it correctly falls through to the existing parsers per research.md §2) —
      NEW `test/test_native/test_tone_command_parser.cpp`
- [X] T007 [P] [US1] Unit tests for `ToneControl::apply()`: returns `true` and writes the
      requested `effect` to `outEffect` when `outputBusy` is `false`; returns `false` (and does
      not touch `outEffect`) when `outputBusy` is `true`, for both a same-effect and a
      different-effect request — NEW `test/test_native/test_tone_control.cpp`
- [X] T008 [US1] Integration test `test_tone_command_to_output_flow.cpp`: via a
      `pumpToneCommand()` helper (mirrors `main.cpp`'s intended dispatch) against `FakeTransport`
      + `ToneControl` + `FakeToneOutput` — a queued `HORN` line with `busy()==false` results in
      exactly one `play(SoundEffectId::Horn)` call; a second `HORN` (and separately, a `SIREN`)
      line queued while `FakeToneOutput::setBusy(true)` results in no additional `play()` call;
      after `setBusy(false)`, the next queued trigger plays again — NEW
      `test/test_native/test_tone_command_to_output_flow.cpp` (depends on T003, T005, T006, T007)

### Implementation for User Story 1

- [X] T009 [US1] Implement `parseToneCommand()` in NEW `src/protocol/tone_command_parser.cpp` per
      `contracts/tone-command-protocol.md` (depends on T003; makes T006 pass)
- [X] T010 [US1] Implement `ToneControl::apply()` in NEW `src/control/tone_control.h` + `.cpp` per
      `data-model.md`'s decision table (depends on T003; makes T007 pass)
- [X] T011 [US1] Implement `PwmToneOutput` (`IToneOutput`) in NEW
      `src/hardware/pwm_tone_output.h`: `begin()` attaches LEDC tone generation to
      `config::kToneOutputPin`; `play(effect)` records the effect and start time; `tick(nowMs)`
      updates the LEDC frequency per the horn/engine/siren behavior in
      `contracts/tone-output-hardware-contract.md` (research.md §6, §7) and stops the tone once
      the effect's configured duration elapses; `busy()` reflects whether `nowMs` is still within
      the active effect's duration window — ESP32-only, excluded from the native test build like
      `pwm_steering_servo.h` (depends on T002, T004)
- [X] T012 [US1] Wire tones into `src/main.cpp`: add `ToneControl toneControl;` and
      `PwmToneOutput toneOutput;` instances; call `toneOutput.begin()` in `setup()` (alongside
      `motorDriver.begin()`/`steeringServo.begin()`/`ledOutput.begin()`/`lightsOutput.begin()`);
      in `loop()`, alongside the existing `parseLightCommand()` check, try
      `parseToneCommand(line, toneCmd)` — on `ParseResult::Ok`, call `toneControl.apply(toneCmd,
      toneOutput.busy(), effect)` and, if it returns `true`, `toneOutput.play(effect)`; skip
      drive-command handling for that line either way; otherwise fall through to the existing
      light/drive handling unchanged; call `toneOutput.tick(millis())` every `loop()` iteration,
      unconditionally (depends on T009, T010, T011)
- [X] T013 [P] [US1] Register `test_tone_command_parser.cpp`'s and `test_tone_control.cpp`'s test
      functions, and the test from `test_tone_command_to_output_flow.cpp`, as `extern`/
      `RUN_TEST` entries in `test/test_native/test_main.cpp`, then run `pio test -e native` and
      confirm all pass (depends on T006–T008) — confirmed: 106/106 passing

**Checkpoint**: User Story 1 is fully functional and independently testable — triggering a sound
effect plays it, and a retrigger while playing is correctly ignored.

---

## Phase 4: User Story 2 - Driving stays fully responsive regardless of sound-effect playback (Priority: P1)

**Goal**: Drive commands are never delayed/dropped by sound-effect playback, and a playing
effect is never cut short by a drive command; existing fail-safe behavior is untouched.

**Independent Test**: Per quickstart.md steps 7–8: send drive commands continuously while a tone
plays and confirm neither is degraded; force a disconnect while a tone plays and confirm the
existing fail-safe timing is unchanged.

### Tests for User Story 2 ⚠️

No new host-testable logic for this story — the non-interference it requires comes from
`PwmToneOutput`'s non-blocking `tick()` design (T011), a real-hardware timing property, not new
conditional logic to unit test. Verified by a full regression run (T014) plus on-device timing
(T015), since "no added delay" and "not cut short" are not things a host test can assert
(plan.md Testing section).

### Implementation for User Story 2

- [X] T014 [US2] Run the full existing native suite (`pio test -e native`) and confirm every
      pre-existing test (drive command parsing, `DirectionControl`, `MotorServoVehicleOutput`,
      lights, control-channel connection logging) still passes unmodified — proving this
      feature's additions (T001–T013) touched no existing file's behavior (plan.md Constitution
      Check, Principle V) (depends on T013) — confirmed: 106/106 passing, all pre-existing tests
      unaffected
- [ ] T015 [US2] On-device validation: run `quickstart.md` steps 7 and 8 — drive commands sent
      while a tone plays produce no added delay versus no tone playing (SC-002, FR-006); the tone
      is not cut short by drive commands (FR-007); a forced disconnect/power-cycle while a tone
      is playing leaves the existing fail-safe timing completely unaffected (SC-004, FR-008) —
      BLOCKED: no ESP32 board available to flash in this environment; needs to be run on-device
      separately (depends on T012)

**Checkpoint**: Both user stories independently functional — sound effects play correctly and
never compromise driving responsiveness or safety.

---

## Phase 5: Polish & Cross-Cutting Concerns

- [X] T016 [P] Add GPIO26 (`kToneOutputPin`, sound effects: horn/engine/siren) to the "Pinout"
      table in `README.md`, and add the speaker/buzzer + volume trim potentiometer to the
      "Electronic components" list, matching the pattern already used for the servo/motor/lights
      pins (depends on T002)
- [X] T017 Build firmware (`pio run -e esp32dev`) and confirm it compiles/links cleanly with the
      tone command parsing, `ToneControl`, `PwmToneOutput`, and `main.cpp` wiring (depends on
      T012) — confirmed: `Linking .pio/build/esp32dev/firmware.elf` succeeds with zero compiler
      errors; the subsequent `esptool.py elf2image` step fails with `FileNotFoundError:
      firmware.elf` on this machine, the same pre-existing local toolchain issue already
      documented in `006`'s tasks.md (T024), not a regression from this feature. This build is
      also what caught and fixed the real LEDC API shape (research.md §8): the installed
      `framework-arduinoespressif32 @ 3.20017.241212` uses the channel-based
      `ledcSetup`/`ledcAttachPin`/`ledcWriteTone` API, not the pin-based `ledcAttach()` form
      initially assumed
- [ ] T018 Run the full on-device `quickstart.md` validation (steps 1–8) end-to-end against a
      flashed `esp32dev` build with the speaker/buzzer wired to `config::kToneOutputPin` (GPIO26)
      — confirms the complete feature in one pass, not just per-story slices — BLOCKED: no ESP32
      board available to flash in this environment; needs to be run on-device separately (depends
      on T001–T015)
- [X] T019 [P] Remove the `SIREN` effect — the car is not an ambulance: drop `Siren` from
      `SoundEffectId` (`src/protocol/tone_command_parser.h`) and its parsing branch
      (`.cpp`); drop `kSirenDurationMs`/`kSirenMinFreqHz`/`kSirenMaxFreqHz` and the
      `sirenFrequency()` case from `PwmToneOutput`; update `test_tone_command_parser.cpp` (replace
      `test_tone_parses_siren` and the `SiReN` case-insensitivity check with an `ENGINE`
      equivalent; add `test_tone_rejects_removed_siren_word_as_malformed` asserting `SIREN` is now
      `Malformed`), `test_tone_control.cpp`, and `test_tone_command_to_output_flow.cpp` (both
      swap their `SIREN` different-effect cases for `ENGINE`); re-register tests in
      `test_main.cpp`; update `README.md`'s pinout/electronics entries to drop "siren" — confirmed:
      `pio test -e native` 106/106 passing (see Post-completion change note below)
- [X] T020 [P] Make the engine tone fully automatic and have the horn layer on top of it instead
      of being mutually exclusive with it: drop the manual `ENGINE` trigger (`parseToneCommand()`
      → `parseHornCommand()`, now horn-only, in `src/protocol/tone_command_parser.h`/`.cpp`); drop
      `SoundEffectId`/`ToneCommand` (no longer needed with a single trigger word); simplify
      `ToneControl::apply()` to `bool apply(bool hornBusy)` (`src/control/tone_control.h`/`.cpp`);
      add `motorEngaged(Direction)` to `src/control/direction.h`; change `IToneOutput` to
      `playHorn()`/`hornBusy()`/`setEngineRunning(bool)`/`tick()` (`src/hardware/i_tone_output.h`);
      rewrite `PwmToneOutput` (`src/hardware/pwm_tone_output.h`) so the engine tone plays
      continuously (idle ~52Hz or running ~147Hz, per `setEngineRunning()`) and the horn, while
      active, time-slices the single LEDC channel between the horn frequency and the current
      engine frequency each `config::kToneLayerPeriodMs` window rather than silencing the engine;
      update `src/config.h` (`kEngineIdleBaseFreqHz`/`kEngineIdleWobbleFreqHz`,
      `kEngineRunningBaseFreqHz`/`kEngineRunningWobbleFreqHz`, `kToneLayerPeriodMs`/
      `kToneLayerHornSliceMs`; drop `kEngineDurationMs`); wire `src/main.cpp` to call
      `toneOutput.setEngineRunning(motorEngaged(direction))` everywhere a `Direction` is decided
      (both the normal command path and the safe-state STOP transition); rewrite
      `FakeToneOutput`, `test_tone_command_parser.cpp`, `test_tone_control.cpp`, and
      `test_tone_command_to_output_flow.cpp` for the new signatures; add
      `test_drive_to_engine_tone_flow.cpp` (new integration test: drive command → `Direction` →
      `motorEngaged()` → `setEngineRunning()`) and `motorEngaged()` unit tests in
      `test_direction_control.cpp`; re-register everything in `test_main.cpp`; update `spec.md`,
      `plan.md`, `research.md`, `data-model.md`, both `contracts/` files, `quickstart.md`, and
      `README.md` — confirmed: `pio test -e native` 112/112 passing; `pio run -e esp32dev` links
      cleanly with zero compiler errors (see Post-completion change note below)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately (T001 cleans up the superseded
  design; T002 repurposes config already in the codebase)
- **Foundational (Phase 2)**: Depends on Setup only loosely (T003/T004 don't need T001/T002, but
  are sequenced after for consistency) — BLOCKS all user stories
- **User Story 1 (Phase 3)**: Depends on Foundational (T003–T005) and Setup (T002)
- **User Story 2 (Phase 4)**: Depends on User Story 1 (T012's wiring must exist to
  regression-test and on-device-validate against) — not independent of US1's code, but
  independently *testable* and *demoable* as its own increment, per spec.md (both are P1; US2 is
  sequenced second because it validates non-interference with what US1 just built)
- **Polish (Phase 6)**: Depends on all desired user stories being complete

### Within Each User Story

- Tests (T006–T008) MUST be written and FAIL before their corresponding implementation tasks
- Types/interfaces (T003, T004) before anything that implements or fakes them
- `ToneControl`/`PwmToneOutput` (T010, T011) before wiring them into `main.cpp` (T012)
- Story complete (implementation + test registration + green `pio test -e native`) before moving
  to the next priority

### Parallel Opportunities

- T002 (Setup) has no file conflict with T001 but is naturally sequenced after it for clarity
- T003 and T004 (Phase 2) touch different files and can run in parallel
- T006 and T007 (Phase 3 tests) touch different files and can run in parallel; T008 depends on
  both being written first
- T009, T010, T011 (Phase 3 implementation) touch different files and can run in parallel once
  their respective Foundational types exist

---

## Parallel Example: User Story 1

```bash
# Launch both unit test files for User Story 1 together:
Task: "Unit tests for parseToneCommand() in test/test_native/test_tone_command_parser.cpp"
Task: "Unit tests for ToneControl in test/test_native/test_tone_control.cpp"

# Once both exist (and fail), implement in parallel: T009 (parser), T010 (control), and T011
# (PwmToneOutput) touch different files and have no dependency on each other; T012 (main.cpp
# wiring) waits on all three.
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001–T002)
2. Complete Phase 2: Foundational (T003–T005)
3. Complete Phase 3: User Story 1 (T006–T013)
4. **STOP and VALIDATE**: `pio test -e native`, then `quickstart.md` steps 2–6 on-device
5. This alone proves triggering a sound effect works with the correct busy/ignore behavior — the
   non-interference guarantee (US2) is validated next without changing this core mechanism

### Incremental Delivery

1. Setup + Foundational → foundation ready (T001–T005)
2. User Story 1 → build + validate trigger/busy behavior (T006–T013) — MVP
3. User Story 2 → validate no interference with drive control or existing safety behavior
   (T014–T015)
4. Polish → README pinout update + firmware build check + full on-device pass (T016–T018)
5. Post-completion → siren effect removed (T019); engine tone made automatic, horn layered on top
   of it instead of mutually exclusive (T020)

---

## Notes

- No changes to `command_parser`, `drive_command_assembler`, `direction_control`,
  `light_command_parser`, `lights_control`, `ITransport`, `BluetoothTransport`, or any existing
  `Hardware` implementation (plan.md Summary)
- Sound effects intentionally do NOT participate in the motor/servo fail-safe (FR-008) — no task
  in this list makes a tone react to a disconnect or timeout; this mirrors `006`'s lights
  precedent, not a gap
- Originally one global busy flag shared by horn and engine retriggers, not per-effect (T007/T008
  as originally written). Superseded by the engine-automation change below: the engine no longer
  has a manual trigger or a busy concept at all, so the busy flag now scopes to the horn only
  (research.md §4, as revised)
- Commit after each task or logical group, per AGENTS.md conventions

### Post-completion change: siren effect removed

After T001–T018 above were implemented and passing, the `SIREN` effect (T003's/T006's/T008's/
T011's original wording above, left unedited for history) was removed per explicit instruction —
the car is not an ambulance. `SoundEffectId` now has only `Horn`/`Engine`; `SIREN` is a
`ParseResult::Malformed` word like any other unrecognized line. Updated: `tone_command_parser.h`/
`.cpp`, `config.h` (dropped `kSirenDurationMs`/`kSirenMinFreqHz`/`kSirenMaxFreqHz`),
`pwm_tone_output.h` (dropped `sirenFrequency()` and its `durationMs()`/`applyFrequency()` cases),
`test_tone_command_parser.cpp`, `test_tone_control.cpp`, `test_tone_command_to_output_flow.cpp`,
`test_main.cpp`'s registrations, `README.md`, and every design doc (spec.md, plan.md,
research.md, data-model.md, both contracts, quickstart.md). Unaffected: the global busy/ignore
mechanism (FR-004/FR-005), the horn's 1500ms fixed duration, and the engine's V8-mimicking
rumble — none of those depended on siren existing.

### Post-completion change: engine tone made automatic, horn layered on top of it

After T001–T019 above (left unedited for history), the engine sound effect was redesigned per
explicit instruction: it is no longer a manual `ENGINE` trigger competing with the horn for one
shared busy flag. It now plays continuously and automatically, switching between an idle rumble
(DC motor not engaged) and a running rumble (DC motor engaged, forward or backward) as
`main.cpp` decides each drive command — see `research.md` §9. The horn, when triggered, now plays
layered on top of whichever engine tone is active (via rapid time-slicing of the single LEDC
channel, research.md §10) rather than being blocked by or blocking the engine.

Removed/renamed: `SoundEffectId`/`ToneCommand` (no longer meaningful with a single trigger word);
`parseToneCommand()` → `parseHornCommand()` (horn-only, no output param);
`ToneControl::apply(command, outputBusy, outEffect)` → `apply(hornBusy) -> bool`;
`IToneOutput::play(effect)`/`busy()` → `playHorn()`/`hornBusy()` plus a new
`setEngineRunning(bool)`; `config::kEngineDurationMs`/`kEngineBaseFreqHz`/`kEngineWobbleFreqHz` →
`kEngineIdleBaseFreqHz`/`kEngineIdleWobbleFreqHz` (idle) and `kEngineRunningBaseFreqHz`/
`kEngineRunningWobbleFreqHz` (running), plus new `kToneLayerPeriodMs`/`kToneLayerHornSliceMs` for
the horn/engine time-slicing.

Added: `motorEngaged(Direction)` in `src/control/direction.h` (pure mapping, unit-tested in
`test_direction_control.cpp`); `test_drive_to_engine_tone_flow.cpp` (integration test for the
automatic engine-state path, mirroring `test_command_to_actuation_flow.cpp`'s style).

Unaffected: the horn's 1500ms fixed duration and 420Hz tone (FR-004), the "never delay drive
commands" non-blocking `tick()` design (FR-011/FR-012, research.md §7), the DC-motor/servo
fail-safe scope exclusion (FR-013), and the single-GPIO-26 hardware choice (no new pin was added —
research.md §10 explicitly considered and rejected a second output pin for true simultaneous
mixing, in favor of time-slicing the existing one).
