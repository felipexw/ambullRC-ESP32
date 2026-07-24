---
description: "Task list for Motor & Servo Actuation"
---

# Tasks: Motor & Servo Actuation

**Input**: Design documents from `/specs/002-motor-servo-actuation/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md,
contracts/direction-to-actuation-contract.md, quickstart.md

**Tests**: Included and REQUIRED — Constitution Principle II (Test-First) is non-negotiable for
this project: unit tests for the Direction→actuation mapping and the reversal-pause timing, plus
an integration test for the full command-in → hardware-out path, are mandatory, not optional.

**Scope reminder** (from plan.md): this feature adds a second `IVehicleOutput` implementation
(`MotorServoVehicleOutput`) that actually drives the DC motor and steering servo from the
`Direction` the existing `Control` layer already decides — wired in `main.cpp` **alongside** the
existing `SerialDirectionOutput`, not instead of it. The standalone hardware bring-up test is
deleted (Clarifications). No changes to the Bluetooth wire protocol or `Direction` decision logic.

**Organization**: Tasks are grouped by user story (spec.md) to enable independent implementation
and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)

## Path Conventions

Single embedded project per plan.md: new files live in the existing `src/hardware/` directory;
tests in `test/test_native/` (unit tests + `fakes/`) and a new integration test file.

---

## Phase 1: Setup

**Purpose**: Clear the way for the new actuation code before building it

- [X] T001 Remove the standalone hardware bring-up test from `src/main.cpp`: the `Servo
      steeringServo` instance, `BringUpStage` enum, `moveServoTo()`, `driveMotor()`, `stopMotor()`,
      `enterBringUpStage()`, `stepBringUpTest()`, and their calls from `setup()`/`loop()` — per
      FR-011 and the Clarifications decision to delete rather than gate it
- [X] T002 [P] Add `kServoLeftAngleDeg = 10`, `kServoRightAngleDeg = 180`, and
      `kMotorReversePauseMs = 300` to `src/config.h`; update the existing "bring-up test" comments
      above the servo/motor pin constants, since `kServoPin`, `kMotorPinA`, `kMotorPinB`, etc. are
      now used by real actuation, not just the (now-deleted) bring-up test

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Shared interfaces and test doubles every user story depends on

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T003 [P] Add a `virtual void tick(unsigned long nowMs) {}` default no-op method to
      `IVehicleOutput` in `src/hardware/i_vehicle_output.h`, per research.md §2
      (`SerialDirectionOutput` needs no change — it inherits the default)
- [X] T004 [P] Define the `IMotorDriver` interface (`driveForward()`, `driveReverse()`, `stop()`)
      in `src/hardware/i_motor_driver.h`
- [X] T005 [P] Define the `ISteeringServo` interface (`setAngleDeg(int angleDeg)`) in
      `src/hardware/i_steering_servo.h`
- [X] T006 [P] Implement `FakeMotorDriver` test double (records each call —
      `driveForward`/`driveReverse`/`stop` — in order, plus a `last()` accessor) in
      `test/test_native/fakes/fake_motor_driver.h`
- [X] T007 [P] Implement `FakeSteeringServo` test double (records the last angle set, plus a
      history if useful for assertions) in `test/test_native/fakes/fake_steering_servo.h`

**Checkpoint**: Foundation ready — user story implementation can now begin

---

## Phase 3: User Story 1 - Drive forward and backward on command (Priority: P1) 🎯 MVP

**Goal**: "up"/"down" commands (`FORWARD`/`BACKWARD`) actually drive the DC motor forward/reverse
through real GPIO output, respecting the 300ms protective pause before reversing polarity, and the
motor stops when neither is decided — the existing serial log is unaffected.

**Independent Test**: Per quickstart.md, send `0,100` / `0,-100` / `0,0` through `FakeTransport` →
… → `FakeMotorDriver` and assert the forward/stop/pause/reverse sequencing and its timing;
confirm `RecordingVehicleOutput` still logs `FORWARD`/`BACKWARD`/`STOP` unchanged.

### Tests for User Story 1 ⚠️

> Write these tests FIRST, ensure they FAIL before implementation

- [X] T008 [P] [US1] Unit tests: `MotorServoVehicleOutput.emit()` calls `driveForward()`
      immediately for `FORWARD`, `driveReverse()` immediately for `BACKWARD`, and `stop()`
      immediately for `STOP`, when starting from `Stopped` — against `FakeMotorDriver`, in NEW
      `test/test_native/test_motor_servo_vehicle_output.cpp`
- [X] T009 [P] [US1] Unit tests: a true reversal (`FORWARD`→`BACKWARD` or vice versa) calls
      `stop()` immediately and does NOT engage the new polarity until `tick(nowMs)` has been called
      with `nowMs - pauseStart >= config::kMotorReversePauseMs` — assert the new polarity is still
      not applied just before the threshold (e.g. 299ms) and is applied at/after 300ms, in
      `test/test_native/test_motor_servo_vehicle_output.cpp`
- [X] T010 [US1] Integration test: NEW `test/test_native/test_command_to_actuation_flow.cpp` —
      `FakeTransport` sends `0,100` then `0,-100`; assert `FakeMotorDriver` shows forward, then an
      immediate stop, then (only after advancing time via `tick()`) reverse, while
      `RecordingVehicleOutput` still logs `FORWARD` then `BACKWARD` unchanged (depends on T006,
      T007)

### Implementation for User Story 1

- [X] T011 [P] [US1] Implement `GpioMotorDriver` (`IMotorDriver`) in NEW
      `src/hardware/gpio_motor_driver.h`: a `begin()` that sets `config::kMotorPinA`/`kMotorPinB`
      to `OUTPUT` and stops, plus `driveForward()`/`driveReverse()`/`stop()` via `digitalWrite`
- [X] T012 [P] [US1] Implement `PwmSteeringServo` (`ISteeringServo`) in NEW
      `src/hardware/pwm_steering_servo.h`: wraps `ESP32Servo`'s `Servo`; `begin()` attaches on
      `config::kServoPin` with `kServoMinPulseUs`/`kServoMaxPulseUs`; `setAngleDeg()` clamps to
      `[kServoMinAngleDeg, kServoMaxAngleDeg]` and calls `Servo::write()`
- [X] T013 [US1] Implement `MotorServoVehicleOutput` in NEW
      `src/hardware/motor_servo_vehicle_output.h` (header-only, matching the other Hardware-layer
      classes): the full `Direction` → (motor polarity, servo angle) decode for all 9 values per
      `contracts/direction-to-actuation-contract.md` in `emit()` (servo angle applied immediately
      every call), and the reversal-pause state machine in `tick()` per data-model.md (depends on
      T003–T005)
- [X] T014 [US1] Wire `GpioMotorDriver` + `PwmSteeringServo` + `MotorServoVehicleOutput` into
      `src/main.cpp`: initialize both drivers in `setup()` (pin/servo init, initial stop/neutral);
      in `loop()`, add a small `emitDirection(Direction)` helper that calls `emit()` on both the
      existing `SerialDirectionOutput` and the new `MotorServoVehicleOutput`, use it at the
      command-arrival branch, and call `hardwareOutput.tick(millis())` unconditionally once per
      loop iteration

**Checkpoint**: User Story 1 is fully functional and independently testable — forward/backward
driving with the reversal pause works end-to-end; serial logging is unaffected.

---

## Phase 4: User Story 2 - Steer left and right on command (Priority: P1)

**Goal**: "left"/"right" (and combined forward/backward+left/right) commands turn the steering
servo to its full-lock angle immediately, independent of the motor's state.

**Independent Test**: Send `-50,0` / `50,0` / `50,100` etc. through the chain and assert
`FakeSteeringServo`'s angle matches `contracts/direction-to-actuation-contract.md`, including while
the motor is mid-reversal-pause.

### Tests for User Story 2 ⚠️

> Write these tests FIRST, ensure they FAIL before implementation

- [X] T015 [P] [US2] Unit tests: `MotorServoVehicleOutput.emit()` sets `kServoLeftAngleDeg` for
      `LEFT`/`FORWARD_LEFT`/`BACKWARD_LEFT`, `kServoRightAngleDeg` for
      `RIGHT`/`FORWARD_RIGHT`/`BACKWARD_RIGHT`, and `kServoNeutralAngleDeg` for the remaining three
      — all 9 rows of the contract table — against `FakeSteeringServo`, in
      `test/test_native/test_motor_servo_vehicle_output.cpp`
- [X] T016 [P] [US2] Unit test: the servo angle updates immediately even while the motor is
      mid-reversal-pause — e.g. `emit(FORWARD)`, then `emit(BACKWARD)` (starts the pause), then
      `emit(FORWARD_RIGHT)` before the pause elapses — `FakeSteeringServo` must already read
      `kServoRightAngleDeg` even though the motor hasn't reversed yet (proves FR-007 independence),
      in `test/test_native/test_motor_servo_vehicle_output.cpp`
- [X] T017 [US2] Integration test: extend `test/test_native/test_command_to_actuation_flow.cpp` —
      send `-50,0` then `50,100`, asserting `FakeSteeringServo`/`FakeMotorDriver` end state matches
      the contract table and `RecordingVehicleOutput` logs `LEFT` then `FORWARD_RIGHT`

### Implementation for User Story 2

- [X] T018 [US2] No new production code required — the full decode table and `PwmSteeringServo`
      were already built in T012/T013 so `main.cpp` would compile for User Story 1; this story is
      verified entirely by T015–T017's new tests passing (same situation as 001's T020: an earlier
      task already covers the behavior, a later story's tests confirm it)

**Checkpoint**: User Stories 1 and 2 both independently functional — driving and steering are
proven correct and independent of each other.

---

## Phase 5: User Story 3 - Fail-safe stop applies to real hardware (Priority: P1)

**Goal**: Disconnect, timeout, or a malformed command now stops the real motor and returns the
real servo to neutral immediately — including when it interrupts an in-progress reversal pause —
not just the log line.

**Independent Test**: Drive forward, disconnect, and confirm `FakeMotorDriver` shows an immediate
stop (not gated behind the 300ms pause) and `FakeSteeringServo` shows neutral; separately, start a
reversal (motor mid-pause) then disconnect and confirm the motor stays stopped through and after
the pause window, never resurrecting the pre-disconnect polarity.

### Tests for User Story 3 ⚠️

> Write these tests FIRST, ensure they FAIL before implementation

- [X] T019 [P] [US3] Unit test: `emit(STOP)` from `Forward` (or `Reverse`) calls `stop()` and sets
      the servo to neutral immediately, with no pause, in
      `test/test_native/test_motor_servo_vehicle_output.cpp`
- [X] T020 [P] [US3] Unit test: `emit(STOP)` arriving while a reversal pause is already in
      progress leaves the motor stopped through and after the pause elapses — `tick()` must not
      apply the pre-disconnect polarity once the pause timer runs out, in
      `test/test_native/test_motor_servo_vehicle_output.cpp`
- [X] T021 [US3] Integration test: extend `test/test_native/test_command_to_actuation_flow.cpp` —
      simulate a disconnect while driving forward, and separately a malformed line mid-drive,
      asserting `FakeMotorDriver`/`FakeSteeringServo` reach/hold the safe state and
      `RecordingVehicleOutput` still logs `STOP` exactly once per transition (reuses
      `DirectionControl`'s existing safe-state logic from `001-bluetooth-motor-control`)

### Implementation for User Story 3

- [X] T022 [US3] Wire `hardwareOutput.emit(safeStateDirection)` into the existing fail-safe branch
      in `src/main.cpp`'s `loop()` (the `control.onTick(...)` branch), alongside the existing
      `output.emit(safeStateDirection)`, using the same `emitDirection()` helper added in T014

**Checkpoint**: All three user stories independently functional per quickstart.md — the fail-safe
now protects real hardware, not just the log.

---

## Phase 6: Polish & Cross-Cutting Concerns

- [X] T023 [P] Reconcile `contracts/direction-to-actuation-contract.md` and `data-model.md` with
      any edge case discovered during implementation — no changes needed to either; the
      implementation matched the contract exactly. `plan.md`/`tasks.md` were updated to reflect
      that `MotorServoVehicleOutput` ended up header-only (like the other Hardware-layer classes)
      rather than `.h`/`.cpp`
- [X] T024 Run `pio run -e esp32dev` and confirm the firmware still builds after removing the
      bring-up test and adding the new hardware classes — SUCCESS, 5.98s, RAM 12.3% (40204 bytes),
      Flash 85.1% (1115317 bytes)
- [X] T025 Run `pio test -e native` and confirm the full suite (existing tests plus T008–T021)
      passes — 50/50 passed
- [ ] T026 Run the on-device validation steps in `quickstart.md` against a flashed `esp32dev`
      build, confirming the reversal-pause timing and the immediate fail-safe stop physically —
      **NOT DONE**: no ESP32 is currently connected to this machine (`pio device list` shows no
      `usbserial` port), and this step needs a phone with an SPP terminal app to actually send
      commands. Requires manual follow-up, same as `001`'s T028.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately
- **Foundational (Phase 2)**: Depends on Setup — BLOCKS all user stories
- **User Story 1 (Phase 3)**: Depends on Foundational only
- **User Story 2 (Phase 4)**: Depends on Foundational and on T012/T013 (built in US1) to compile
  and to have something to test — implement after US1
- **User Story 3 (Phase 5)**: Depends on Foundational and on T013/T014 (built in US1) — implement
  after US1 (and, for a clean diff, after US2)
- **Polish (Phase 6)**: Depends on all desired user stories being complete

### Within Each User Story

- Tests MUST be written and FAIL before implementation (Constitution Principle II)
- Interfaces/fakes (Foundational) before the `MotorServoVehicleOutput` logic that uses them
- `MotorServoVehicleOutput` logic before `main.cpp` wiring
- `main.cpp` wiring last, once the pieces it wires together exist

### Parallel Opportunities

- All Foundational tasks marked [P] (T003–T007) can run in parallel — different files, no
  cross-dependencies
- T011 and T012 (the two real ESP32 driver wrappers) can run in parallel — different files
- Within each story, unit test tasks marked [P] can run in parallel with each other; the
  integration test depends on the fakes and comes last among tests
- T023 (docs) can run in parallel with T024–T026 (validation)

---

## Parallel Example: User Story 1

```bash
# Unit tests for US1 together:
Task: "Unit tests: FORWARD/BACKWARD/STOP apply immediately from Stopped in test/test_native/test_motor_servo_vehicle_output.cpp"
Task: "Unit tests: true reversal stops immediately then waits 300ms in test/test_native/test_motor_servo_vehicle_output.cpp"

# Real ESP32 driver wrappers together:
Task: "Implement GpioMotorDriver in src/hardware/gpio_motor_driver.h"
Task: "Implement PwmSteeringServo in src/hardware/pwm_steering_servo.h"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (blocks everything)
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: `pio test -e native`, then the forward/backward/stop rows of
   quickstart.md on-device
5. This is the smallest slice that proves a command actually moves the car

### Incremental Delivery

1. Setup + Foundational → foundation ready
2. User Story 1 → validate independently (MVP: forward/backward driving with the safety pause)
3. User Story 2 → validate independently (steering left/right, independent of the motor)
4. User Story 3 → validate independently (fail-safe now stops real hardware, even mid-pause)
5. Polish → full native suite + full quickstart.md pass, including on-device validation

---

## Notes

- No changes to the Bluetooth wire protocol, `command_parser`, or `direction_control` — this
  feature only adds a `Hardware`-layer consumer of the `Direction` those already produce (per
  plan.md Summary)
- Commit after each task or logical group, per AGENTS.md conventions
- Verify each test fails before implementing the corresponding production code
