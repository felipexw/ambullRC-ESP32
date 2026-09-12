---
description: "Task list for Four-Light Auxiliary Control"
---

# Tasks: Four-Light Auxiliary Control

**Input**: Design documents from `/specs/006-four-led-lights-control/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md,
contracts/light-command-protocol.md, contracts/lights-state-to-gpio-contract.md, quickstart.md

**Tests**: Included and REQUIRED — Constitution Principle II (Test-First) is non-negotiable for
this project: unit tests for `parseLightCommand()`, `LightsControl`, and `formatLightsReport()`,
plus integration tests for the full received-line → applied-state → written-report path, are
mandatory, not optional. `GpioLightsOutput` itself is a thin ESP32-only GPIO wrapper (same shape
as `GpioMotorDriver`/`LedConnectionOutput`) and is validated on-device via `quickstart.md` instead
of a host unit test — established precedent, not a gap (see plan.md's Constitution Check).

**Scope reminder** (from plan.md): this feature adds eight new word commands
(`LIGHT<n>ON`/`LIGHT<n>OFF`) alongside the existing drive-command vocabulary, a new
`LightsControl` (Control layer), a new `ILightsOutput`/`GpioLightsOutput` (Hardware layer), and one
new capability the protocol has never needed before — the ESP32 sending data back to the app —
via a new `ITransport::writeLine()`. No changes to `Direction`, `ConnectionEvent`, the DC
motor/servo actuation, or any existing wire-format word/numeric pair.

**Organization**: Tasks are grouped by user story (spec.md) to enable independent implementation
and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)

## Path Conventions

Single embedded project per plan.md: new files live in the existing `src/protocol/`,
`src/control/`, `src/hardware/`, and `src/transport/` directories; tests in `test/test_native/`
(unit tests + `fakes/`) and one new integration test file.

---

## Phase 1: Setup

**Purpose**: Add the config values the new code needs before it exists

- [X] T001 Add `kLightCount = 4` and `kLight1Pin`..`kLight4Pin` (GPIO 21, 22, 23, 25 — per
      research.md §6: non-strapping, not input-only, not already in use) to `src/config.h`, in a
      new "Auxiliary lights (Hardware layer: GpioLightsOutput)" section near the other pin
      constants

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Shared types/interfaces/test doubles every user story depends on

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T002 [P] Define `struct LightCommand { int lightId; bool on; };`, `using LightsState =
      std::array<bool, config::kLightCount>;`, and declare `ParseResult parseLightCommand(const
      std::string& line, LightCommand& out);` in NEW `src/protocol/light_command_parser.h` (per
      research.md §5 and data-model.md — types live in Protocol, not Control)
- [X] T003 [P] Define the `ILightsOutput` interface (`virtual void apply(const LightsState&
      state) = 0;`) in NEW `src/hardware/i_lights_output.h`
- [X] T004 [P] Implement `RecordingLightsOutput` test double (records every `apply()` call's
      `LightsState` in order, plus a `last()` accessor) in NEW
      `test/test_native/fakes/recording_lights_output.h`

**Checkpoint**: Foundation ready — user story implementation can now begin

---

## Phase 3: User Story 1 - Turn an individual light on or off from the app (Priority: P1) 🎯 MVP

**Goal**: `LIGHT<n>ON`/`LIGHT<n>OFF` commands change only the targeted light's physical GPIO
output; the other three lights are unaffected; repeating the same command is a no-op; all four
lights default OFF at boot.

**Independent Test**: Per quickstart.md steps 2–4 and 6: confirm all lights start OFF, send
`LIGHT1ON` and confirm only light 1 turns on, then send `LIGHT1ON` again and confirm no change —
without needing the state-report behavior (US2/US3) implemented.

### Tests for User Story 1 ⚠️

> Write these tests FIRST, ensure they FAIL before implementation

- [X] T005 [P] [US1] Unit tests for `parseLightCommand()`: `Ok` with the correct `lightId`/`on`
      for all 8 words (`LIGHT1ON`..`LIGHT4OFF`), case-insensitively; `Malformed` for every existing
      drive word (`UP`, `LEFT`, ...), a numeric pair (`"0,0"`), and garbage (so it correctly falls
      through to the existing drive parser per research.md §3); `OutOfRange` for `LIGHT0ON` and
      `LIGHT5ON`..`LIGHT9OFF` — NEW `test/test_native/test_light_command_parser.cpp`
- [X] T006 [P] [US1] Unit tests for `LightsControl`: `state()` defaults to all-`false`; applying
      `{lightId=N, on=true}` changes only index `N-1` and returns `true`; applying the same command
      again returns `false` and leaves `state()` unchanged; each of the 4 lights is independently
      switchable — NEW `test/test_native/test_lights_control.cpp`
- [X] T007 [US1] Integration test `test_light_toggle_flow_only_targeted_light_changes`: via a
      `pumpLightCommand()` helper (mirrors main.cpp's intended dispatch) against `FakeTransport` +
      `LightsControl` + `RecordingLightsOutput` — `LIGHT2ON` results in exactly one `apply()` call
      with `{false, true, false, false}`; a non-light line (`"UP"`) results in no `apply()` call at
      all — NEW `test/test_native/test_light_command_to_report_flow.cpp` (depends on T002, T004,
      T005, T006)

### Implementation for User Story 1

- [X] T008 [US1] Implement `parseLightCommand()` in NEW `src/protocol/light_command_parser.cpp`
      per `contracts/light-command-protocol.md` (depends on T002; makes T005 pass)
- [X] T009 [US1] Implement `LightsControl` (`apply()`, `state()`) in NEW
      `src/control/lights_control.h` + `.cpp` per `data-model.md`'s state machine (depends on T002;
      makes T006 pass)
- [X] T010 [US1] Implement `GpioLightsOutput` (`ILightsOutput`) in NEW
      `src/hardware/gpio_lights_output.h`: `begin()` sets `kLight1Pin`..`kLight4Pin` to `OUTPUT` and
      writes `LOW` (FR-006); `apply(state)` writes each pin `HIGH`/`LOW` from `state[0..3]` per
      `contracts/lights-state-to-gpio-contract.md` (depends on T001, T003)
- [X] T011 [US1] Wire lights into `src/main.cpp`: add `LightsControl lightsControl;` and
      `GpioLightsOutput lightsOutput;` instances; call `lightsOutput.begin()` in `setup()`
      (alongside `motorDriver.begin()`/`steeringServo.begin()`/`ledOutput.begin()`); in `loop()`,
      before the existing `commandAssembler.apply(line, cmd)` call, try `parseLightCommand(line,
      lightCmd)` — on `ParseResult::Ok`, call `lightsControl.apply(lightCmd, lightsState)` then
      `lightsOutput.apply(lightsState)` and skip the drive-command handling for that line;
      otherwise fall through to the existing drive-command handling unchanged (depends on T008,
      T009, T010)
- [X] T012 [P] [US1] Register `test_light_command_parser.cpp`'s and `test_lights_control.cpp`'s
      test functions, and the US1 test from `test_light_command_to_report_flow.cpp`, as
      `extern`/`RUN_TEST` entries in `test/test_native/test_main.cpp`, then run `pio test -e
      native` and confirm all pass (depends on T005–T007)

**Checkpoint**: User Story 1 is fully functional and independently testable — toggling any one
light works correctly with the others unaffected, and the default boot state is OFF.

---

## Phase 4: User Story 2 - App is told the current state after every change (Priority: P1)

**Goal**: Whenever a light command actually changes a light's state, the ESP32 sends
`LIGHTS:bbbb` (current state of all four lights) back to the app; a no-op command sends nothing.

**Independent Test**: Per quickstart.md steps 4 and 6: send `LIGHT1ON` and confirm the terminal
receives `LIGHTS:1000`; send `LIGHT1ON` again and confirm no new `LIGHTS:` line is sent —
independent of how the physical toggle (US1) was built, reusing it as-is.

### Tests for User Story 2 ⚠️

> Write these tests FIRST, ensure they FAIL before implementation

- [X] T013 [P] [US2] Unit tests for `formatLightsReport()`: all-`false` → `"LIGHTS:0000"`; mixed
      states → correct bit order and values (e.g. `{true,false,false,true}` →
      `"LIGHTS:1001"`) — NEW `test/test_native/test_lights_report_formatter.cpp`
- [X] T014 [P] [US2] Extend `FakeTransport` with a `writeLine(const std::string& line)` override
      that records every written line (`writtenLines_` vector + `written()` accessor) in
      `test/test_native/fakes/fake_transport.h`
- [X] T015 [US2] Integration test `test_light_toggle_flow_sends_report_only_on_change`: extend
      `pumpLightCommand()` (from T007) to also call `transport.writeLine(formatLightsReport(state))`
      whenever `LightsControl::apply()` returns `true`; assert `FakeTransport::written()` contains
      exactly one `"LIGHTS:1000"` after `LIGHT1ON`, and gains no new entry after re-sending
      `LIGHT1ON` (no-op) — extends `test/test_native/test_light_command_to_report_flow.cpp`
      (depends on T007, T013, T014)

### Implementation for User Story 2

- [X] T016 [P] [US2] Add `virtual void writeLine(const std::string& line) = 0;` to `ITransport` in
      `src/transport/i_transport.h`
- [X] T017 [US2] Implement `writeLine()` in `src/transport/bluetooth_transport.h` via
      `bt_.println(line.c_str())` (depends on T016)
- [X] T018 [P] [US2] Implement `formatLightsReport(const LightsState& state)` in NEW
      `src/protocol/lights_report_formatter.h` + `.cpp` per `contracts/light-command-protocol.md`
      (depends on T002; makes T013 pass)
- [X] T019 [US2] Wire the report send into `src/main.cpp`'s light-command branch (from T011): when
      `lightsControl.apply()` returns `true`, call
      `transport.writeLine(formatLightsReport(lightsState))` (depends on T011, T017, T018)
- [X] T020 [P] [US2] Register `test_lights_report_formatter.cpp`'s test functions and the US2 test
      from `test_light_command_to_report_flow.cpp` in `test/test_native/test_main.cpp`, then run
      `pio test -e native` and confirm all pass (depends on T013, T015)

**Checkpoint**: User Stories 1 and 2 both independently functional — toggling a light both moves
the physical output and reports the new full state back to the app exactly once per real change.

---

## Phase 5: User Story 3 - App syncs all light states on (re)connection (Priority: P2)

**Goal**: When a new Bluetooth connection is established (including reconnection), the ESP32 sends
the current `LIGHTS:bbbb` state to the app without any light command being sent.

**Independent Test**: Per quickstart.md step 8: set a mixed light state, disconnect and reconnect
the terminal app, and confirm it immediately receives the correct `LIGHTS:bbbb` line with no
command needed — independent of US1/US2's command-driven reporting.

### Tests for User Story 3 ⚠️

> Write this test FIRST, ensure it FAILS before implementation

- [X] T021 [US3] Integration test `test_lights_report_sent_on_connect`: apply a few light commands
      to reach a mixed state, then simulate `ConnectionMonitor` observing
      `transport.setConnected(true)` (a fresh `Connected` transition) and assert
      `transport.written()` gained exactly one line, `formatLightsReport(lightsControl.state())`,
      with no light command sent — extends
      `test/test_native/test_light_command_to_report_flow.cpp` (depends on T007, T015)

### Implementation for User Story 3

- [X] T022 [US3] Wire the connect-resync into `src/main.cpp`'s existing `connectionEvent !=
      ConnectionEvent::None` dispatch: on `ConnectionEvent::Connected`, additionally call
      `transport.writeLine(formatLightsReport(lightsControl.state()))` (depends on T011, T017,
      T018, T019)
- [X] T023 [P] [US3] Register the new test function from `test_light_command_to_report_flow.cpp`
      in `test/test_native/test_main.cpp`, then run `pio test -e native` and confirm all pass
      (depends on T021)

**Checkpoint**: All three user stories independently functional — the four lights can be
individually controlled, every real change is reported, and a fresh/re-connection always
re-syncs the app to the true current state.

---

## Phase 6: Polish & Cross-Cutting Concerns

- [X] T024 [P] Run `pio run -e esp32dev` and confirm the firmware still builds after adding the
      light command parsing/formatting, `LightsControl`, `GpioLightsOutput`, and `writeLine()` —
      compiled and linked successfully (`Linking .pio/build/esp32dev/firmware.elf`, size-checked);
      the subsequent `esptool.py elf2image` step fails with
      `FileNotFoundError: firmware.elf` on this machine, but this is a pre-existing local
      toolchain issue (confirmed by reproducing it identically on unmodified `main` via
      `git stash`), not a regression from this feature — the original static `constexpr int
      kPins[]` array member (T010) was changed to a `static int pin(int index)` accessor after an
      unrelated real linker error (`undefined reference to GpioLightsOutput::kPins`, an ODR-use
      issue with the pre-C++17 toolchain) surfaced it during this task
- [ ] T025 Run the full on-device `quickstart.md` validation (steps 1–9) against a flashed
      `esp32dev` build with four LEDs wired to `config::kLight1Pin`..`kLight4Pin` — BLOCKED: no
      ESP32 board available to flash in this environment; needs to be run on-device separately

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately
- **Foundational (Phase 2)**: Depends on Setup (T001, for `config::kLightCount`) — BLOCKS all user
  stories
- **User Story 1 (Phase 3)**: Depends on Foundational (T002–T004) only
- **User Story 2 (Phase 4)**: Depends on User Story 1 (reuses `pumpLightCommand()`/T007's fixture
  and `LightsControl`/`GpioLightsOutput` wiring from T011) — not independent of US1's code, but
  independently *testable* and *demoable* as its own increment
- **User Story 3 (Phase 5)**: Depends on User Story 2 (reuses `formatLightsReport()`/`writeLine()`
  from T017/T018) — independently testable/demoable as its own increment
- **Polish (Phase 6)**: Depends on all desired user stories being complete

### Within Each User Story

- Tests (T005–T007, T013–T015, T021) MUST be written and FAIL before their corresponding
  implementation tasks
- Types/interfaces (T002–T004) before anything that implements or fakes them
- `LightsControl`/`GpioLightsOutput` (T009, T010) before wiring them into `main.cpp` (T011)
- `ITransport::writeLine()` (T016) before its implementation (T017) before anything that calls it
  (T019, T022)
- Story complete (implementation + test registration + green `pio test -e native`) before moving
  to the next priority

### Parallel Opportunities

- T002, T003, T004 (Phase 2) touch different files and can run in parallel
- T005 and T006 (Phase 3 tests) touch different files and can run in parallel; T007 depends on
  both being written first (but can be written in parallel with T008–T010 being implemented,
  since it's expected to fail until they land)
- T013 and T014 (Phase 4) touch different files and can run in parallel
- T016 and T018 (Phase 4 implementation) touch different files and can run in parallel
- T012, T020, T023 (test-registration/regression-run tasks) have no file conflicts with
  same-phase implementation tasks but are naturally last in their phase

---

## Parallel Example: User Story 1

```bash
# Launch both unit test files for User Story 1 together:
Task: "Unit tests for parseLightCommand() in test/test_native/test_light_command_parser.cpp"
Task: "Unit tests for LightsControl in test/test_native/test_lights_control.cpp"

# Once both exist (and fail), write the integration test, then implement in dependency order:
# T008 (parser) and T009 (control) can be implemented in parallel; T010 (GpioLightsOutput) only
# needs T001/T003, also parallelizable with T008/T009; T011 (main.cpp wiring) waits on all three.
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001)
2. Complete Phase 2: Foundational (T002–T004)
3. Complete Phase 3: User Story 1 (T005–T012)
4. **STOP and VALIDATE**: `pio test -e native`, then quickstart.md steps 2–4 and 6 on-device
5. This alone proves each light can be independently switched on/off with the correct default
   boot state — the app-facing feedback (US2/US3) is layered on next without changing this core
   mechanism

### Incremental Delivery

1. Setup + Foundational → foundation ready (T001–T004)
2. User Story 1 → build + validate physical toggle behavior (T005–T012) — MVP
3. User Story 2 → build + validate change-triggered reporting (T013–T020)
4. User Story 3 → build + validate connect-time resync (T021–T023)
5. Polish → firmware build check + full on-device quickstart.md pass (T024–T025)

---

## Notes

- No changes to `Direction`, `ConnectionEvent`, `DriveCommandAssembler`, the DC motor/servo
  actuation, or any existing drive word/numeric wire form — this feature only adds new commands,
  a new outbound report, and the one new `ITransport::writeLine()` capability that makes it
  possible (per plan.md Summary)
- Lights intentionally do NOT participate in the motor/servo fail-safe (FR-009) — no task in this
  list makes a light react to `ConnectionEvent::Disconnected` or a command timeout; this is a
  deliberate omission, not a gap (see research.md and Constitution Check in plan.md)
- Commit after each task or logical group, per AGENTS.md conventions

### Post-completion change: outgoing report format simplified

After all tasks above were implemented and passing, the outgoing report format was changed from
the per-light `LIGHTS:bbbb` bitmask (T013's/T015's/T021's original wording above, left unedited
for history) to a two-value aggregate: `LIGHT_ON` iff all four lights are on, `LIGHT_OFF`
otherwise — the app only needs to know whether every light is on, not each light's individual
state. Updated: `formatLightsReport()`, its unit tests, the integration test's report
assertions, `contracts/light-command-protocol.md`, and `quickstart.md`. Unaffected: the
"report only when a light's state actually changes" trigger (T011/T019/T022's wiring), which
still fires on any real per-light toggle regardless of whether the aggregate string happens to
stay the same.
