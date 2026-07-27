---
description: "Task list for Connection Status LED"
---

# Tasks: Connection Status LED

**Input**: Design documents from `/specs/004-connection-status-led/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md,
contracts/connection-status-led-signal-contract.md, quickstart.md

**Tests**: The event contract this feature depends on (`ConnectionMonitor` emits `Connected`/
`Disconnected` exactly once per transition) is already covered by the existing
`test_connection_monitor.cpp` and `test_connection_logging_flow.cpp` — unchanged by this feature.
Per plan.md's Constitution Check (Principle II), `LedConnectionOutput` itself is a thin ESP32-only
GPIO wrapper (same shape as `GpioMotorDriver`/`PwmSteeringServo`) and is validated on-device via
`quickstart.md`, not with a new host unit test — this is established precedent from
`002-motor-servo-actuation`, not a gap. No new test files are added; the existing suite is the
regression gate.

**Scope reminder** (from plan.md): this feature adds a second `IConnectionOutput` implementation
(`LedConnectionOutput`) that drives an external LED's GPIO signal from the `ConnectionEvent` the
existing `Control` layer already produces — wired in `main.cpp` **alongside** the existing
`SerialConnectionOutput`, not instead of it. No changes to `Transport`/`Protocol`/`Control`, the
Bluetooth wire protocol, or `ConnectionEvent`/`ConnectionMonitor`.

**Organization**: Tasks are grouped by user story (spec.md) to enable independent implementation
and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)

## Path Conventions

Single embedded project per plan.md: the new file lives in the existing `src/hardware/` directory;
no new test files are needed (see Tests note above).

---

## Phase 1: Setup

**Purpose**: Add the config value the new hardware class needs before it exists

- [X] T001 Add `kLedPin` to `src/config.h`, in a new "Connection status LED (Hardware layer:
      LedConnectionOutput)" section near the other pin constants (`kServoPin`, `kMotorPinA/B`).
      Uses GPIO 14 per user direction (non-strapping, not already in use), with a comment
      referencing research.md §3 (avoids GPIO0/2/5/12/15 to not interfere with boot mode selection)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Shared interfaces/test doubles every user story would depend on

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

No tasks needed: this feature reuses the existing `IConnectionOutput` interface and
`RecordingConnectionOutput` fake unchanged (research.md §2) — there is nothing new to build here
before user story work can start. Proceed directly to Phase 3 once T001 is done.

**Checkpoint**: Foundation ready (config value in place) — user story implementation can now begin

---

## Phase 3: User Story 1 - LED lights up on successful pairing (Priority: P1) 🎯 MVP

**Goal**: When the app successfully connects/pairs over Bluetooth, the external LED's GPIO signal
switches ON (3.3V), with no change to the existing serial connection log.

**Independent Test**: Per quickstart.md steps 1–3: flash, confirm the LED starts OFF, connect an
SPP terminal app, and confirm the LED switches ON at the same time `BLUETOOTH_CONNECTED ...`
prints — without needing the disconnect behavior (User Story 2) implemented.

### Implementation for User Story 1

- [X] T002 [US1] Implement `LedConnectionOutput` (`IConnectionOutput`) in NEW
      `src/hardware/led_connection_output.h`: `begin()` sets `pinMode(config::kLedPin, OUTPUT)` and
      `digitalWrite(config::kLedPin, LOW)` (default OFF, FR-003); `emit(ConnectionEvent event,
      const std::string& deviceId)` sets `digitalWrite(config::kLedPin, HIGH)` on `Connected`,
      `LOW` on `Disconnected`, and no-ops on `None` — the full mapping table from
      `contracts/connection-status-led-signal-contract.md` (depends on T001)
- [X] T003 [US1] Wire `LedConnectionOutput` into `src/main.cpp`: add a `LedConnectionOutput
      ledOutput;` instance alongside the existing `SerialConnectionOutput connectionOutput;`, call
      `ledOutput.begin()` in `setup()` (alongside `motorDriver.begin()`/`steeringServo.begin()`),
      and call `ledOutput.emit(connectionEvent, transport.deviceId())` immediately after the
      existing `connectionOutput.emit(connectionEvent, transport.deviceId())` call in `loop()`'s
      connection-event dispatch (depends on T002)
- [X] T004 [P] [US1] Run `pio test -e native` and confirm the full existing suite still passes
      unchanged — this story adds no new host-testable logic, so this is the regression gate for
      T002/T003 (per the Tests note above). Confirmed: all 66 existing test cases pass unchanged.

**Checkpoint**: User Story 1 is fully functional — the LED turns ON on a successful connection,
independently testable via quickstart.md steps 1–3.

---

## Phase 4: User Story 2 - LED turns off when connection is lost (Priority: P1)

**Goal**: When the Bluetooth connection drops, the LED's GPIO signal switches OFF (0V).

**Independent Test**: Per quickstart.md step 4: with the LED already ON from a prior connection,
disconnect the terminal app and confirm the LED switches OFF at the same time
`BLUETOOTH_DISCONNECTED ...` prints — independent of how the ON behavior (User Story 1) was built.

### Implementation for User Story 2

- [ ] T005 [US2] No new production code required — `emit()`'s `Disconnected` branch was already
      built in T002 and is already wired into the same dispatch call in T003 (same situation as
      `002-motor-servo-actuation`'s Phase 4/T018: an earlier task already covers the behavior).
      This story is verified by running quickstart.md step 4 on-device and confirming the LED
      switches OFF — **NOT DONE**: no ESP32 is currently connected to this machine (`pio device
      list` shows no `usbserial` port). Requires manual follow-up with a flashed board and a phone
      running an SPP terminal app, same as `002-motor-servo-actuation`'s T026.

**Checkpoint**: User Stories 1 and 2 both independently functional — the LED correctly tracks both
directions of a single connect/disconnect cycle.

---

## Phase 5: User Story 3 - LED reflects correct state on power-up and repeated cycles (Priority: P2)

**Goal**: The LED defaults OFF at boot (before any connection has ever been made) and correctly
tracks multiple connect/disconnect/reconnect cycles over a session, with no stuck or stale state.

**Independent Test**: Per quickstart.md steps 2, 5, and 7: confirm the LED is OFF immediately after
flashing/boot, before any connection; then repeat connect→disconnect at least 5 times and confirm
the LED matches the actual state after every cycle; then power-cycle while connected and confirm
the LED comes back up OFF.

### Implementation for User Story 3

- [ ] T006 [US3] No new production code required — `begin()`'s default-OFF write (T002) already
      satisfies the boot-time default, and `ConnectionMonitor`'s existing edge-detection (unchanged
      by this feature — fires exactly once per real transition, never repeats a stale event) already
      guarantees every connect/disconnect cycle reaches `LedConnectionOutput::emit()` via T003's
      wiring. This story is verified by running quickstart.md steps 2, 5, and 7 on-device —
      **NOT DONE**: same hardware-unavailable reason as T005.

**Checkpoint**: All three user stories independently functional — the LED is a reliable connection
indicator across the full power-on-to-shutdown lifecycle, per quickstart.md.

---

## Phase 6: Polish & Cross-Cutting Concerns

- [X] T007 Run `pio run -e esp32dev` and confirm the firmware still builds after adding
      `LedConnectionOutput` and wiring it into `main.cpp` — SUCCESS, 4.26s, RAM 12.3% (40252
      bytes), Flash 85.2% (1116257 bytes)
- [ ] T008 Run the full on-device `quickstart.md` validation end-to-end (steps 1–7) against a
      flashed `esp32dev` build with an LED wired to `config::kLedPin` (GPIO 14) — requires physical
      hardware and a phone with an SPP terminal app; note if hardware is unavailable, same as
      `002-motor-servo-actuation`'s T026 — **NOT DONE**: no ESP32 currently connected.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately
- **Foundational (Phase 2)**: No new tasks — nothing blocks starting Phase 3 beyond T001
- **User Story 1 (Phase 3)**: Depends on Setup (T001) only
- **User Story 2 (Phase 4)**: Depends on User Story 1 (T002/T003 already implement and wire the
  `Disconnected` branch) — verify after US1
- **User Story 3 (Phase 5)**: Depends on User Story 1 (T002/T003 already implement `begin()`'s
  default and the per-cycle wiring) — verify after US1 (and, for a clean incremental story, after
  US2)
- **Polish (Phase 6)**: Depends on all desired user stories being complete

### Within Each User Story

- T002 (the `LedConnectionOutput` class) before T003 (wiring it into `main.cpp`)
- T003 before T004 (regression test only meaningful once the code compiles and is wired in)
- User Stories 2 and 3 are verification-only phases — no new code, per the "no new production
  code required" tasks above

### Parallel Opportunities

- T004 (native test run) has no file conflicts with other tasks and could be kicked off as soon as
  T003 lands, marked `[P]` for visibility, though in practice it's the last step of a short serial
  chain (T001 → T002 → T003 → T004)
- T007 and T008 can be prepared in parallel (build vs. on-device validation) but T008 is more
  useful once T007 confirms the build succeeds

---

## Parallel Example: User Story 1

```bash
# T001 -> T002 -> T003 are strictly sequential (each needs the previous file's contents).
# T004 can be run as soon as T003 is done:
Task: "Run pio test -e native and confirm the full existing suite still passes unchanged"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001)
2. Skip Phase 2: nothing to do
3. Complete Phase 3: User Story 1 (T002–T004)
4. **STOP and VALIDATE**: `pio test -e native`, then quickstart.md steps 1–3 on-device
5. This alone proves the LED turns ON correctly on a successful connection — the OFF path and
   repeated-cycle robustness (US2/US3) are verified next, but the core mechanism is already
   built and doesn't change

### Incremental Delivery

1. Setup → foundation ready (T001)
2. User Story 1 → build + validate ON behavior (T002–T004) — MVP
3. User Story 2 → validate OFF behavior on-device (T005, no new code)
4. User Story 3 → validate boot-default and repeated-cycle behavior on-device (T006, no new code)
5. Polish → firmware build check + full on-device quickstart.md pass (T007–T008)

---

## Notes

- No changes to `ConnectionEvent`, `ConnectionMonitor`, `SerialConnectionOutput`, or the Bluetooth
  wire protocol — this feature only adds a second `Hardware`-layer consumer of the `ConnectionEvent`
  those already produce (per plan.md Summary)
- The LED intentionally does NOT react to `DirectionControl`'s command-timeout safe-state — see
  research.md §1 and quickstart.md step 6
- Commit after each task or logical group, per AGENTS.md conventions
- T005/T006 exist as explicit checklist entries (not silently skipped) so the story's independent
  test is still verified and recorded, even though no production code changes
