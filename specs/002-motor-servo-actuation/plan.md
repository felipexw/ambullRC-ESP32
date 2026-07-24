# Implementation Plan: Motor & Servo Actuation

**Branch**: `002-motor-servo-actuation` | **Date**: 2026-07-23 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/002-motor-servo-actuation/spec.md`

## Summary

The `Control` layer already decides a `Direction` (e.g. `FORWARD`, `BACKWARD_LEFT`, `STOP`) from
each Bluetooth drive command and prints it via the existing `SerialDirectionOutput` — no motor or
servo actually moves (that was the deliberate scope cut of `001-bluetooth-motor-control`).

This plan adds a second `Hardware`-layer implementation of `IVehicleOutput`,
`MotorServoVehicleOutput`, that translates the same `Direction` into real DC-motor and servo
actuation, wired in `main.cpp` **alongside** (not instead of) the existing serial logging. It
drives the DC motor forward/reverse/stopped and the steering servo to a full-left/neutral/
full-right angle per `contracts/direction-to-actuation-contract.md`, inserts the clarified 300ms
protective pause before reversing the motor's polarity, and makes the existing fail-safe
(disconnect/timeout/malformed command) stop real hardware instead of just logging `STOP`. The
standalone hardware bring-up test in `main.cpp` (servo sweep + forward/pause/reverse motor test)
is deleted, per the `/speckit-clarify` decision recorded in the spec.

## Technical Context

**Language/Version**: C++ (Arduino core for ESP32, C++17 where the toolchain allows), via
PlatformIO — same as `001-bluetooth-motor-control`.

**Primary Dependencies**: `madhephaestus/ESP32Servo` (already a declared dependency, used by the
new `PwmSteeringServo`). No new third-party dependency. PlatformIO Unity for tests.

**Storage**: N/A (no persisted state; all state is in-memory: the `Direction` decision and the
motor's applied/desired polarity for the duration of a connection).

**Testing**: Unity via `pio test -e native`, extending `test/test_native` with unit tests for
`MotorServoVehicleOutput` (Direction → actuation mapping, and the non-blocking reversal-pause
state machine, against `FakeMotorDriver`/`FakeSteeringServo`) and an integration test that drives a
fake transport through to both the existing `RecordingVehicleOutput` (serial) and the new fakes,
including a disconnect/timeout scenario.

**Target Platform**: ESP32 Dev Module (`esp32dev`, Arduino framework) for firmware; `native` host
environment for tests (Constitution Principle II).

**Project Type**: Single embedded firmware project (existing PlatformIO layout: `src/`, `test/`).

**Performance Goals**: Per spec SC-001/SC-002 — a decided direction produces a motor/servo
response within 250ms, or within 550ms when the command reverses the motor's current polarity
(250ms base response + the mandatory 300ms protective pause).

**Constraints**: No new wire-protocol or command vocabulary (reuses `001`'s `<steer>,<throttle>`
protocol and `Direction` decision unchanged). Steering is discrete (full-left / neutral /
full-right), not proportional. Must build/run fully on host for tests, without a physical ESP32 —
the new `GpioMotorDriver`/`PwmSteeringServo` real implementations are ESP32-only (like
`BluetoothTransport`) and are exercised only via the on-device quickstart, not host tests.

**Scale/Scope**: Single ESP32, single DC motor, single steering servo, per spec Assumptions.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Check | Result |
|-----------|-------|--------|
| I. Simplicity First (YAGNI) | Only the minimal new pieces this feature needs are added: two small hardware interfaces (`IMotorDriver`, `ISteeringServo`), their ESP32 implementations, and one orchestration class (`MotorServoVehicleOutput`) with a state machine directly required by FR-010's reversal pause — not speculative. No new indirection layer; existing `Transport → Protocol → Control → Hardware` is unchanged. The standalone bring-up test is deleted rather than kept around unused (per Clarifications). | PASS |
| II. Test-First | Unit tests cover the Direction→actuation mapping and the reversal-pause timing (using fakes, no physical hardware or real delay). An integration test covers fake-transport-in → both serial log and motor/servo fakes updated, including the fail-safe path. | PASS |
| III. Simple, Layered Architecture | Only the `Hardware` layer changes. `Control` still only ever produces/consumes a `Direction`; it has no knowledge of motor pins, servo angles, or the reversal pause — that electrical concern stays in `Hardware`, where it belongs. | PASS |
| IV. Hardware Abstraction for Testability | `IMotorDriver` and `ISteeringServo` sit between `MotorServoVehicleOutput` and the real GPIO/servo calls, exactly like `ITransport` and `IVehicleOutput` already do for Bluetooth and logging. Fakes exist for both in `test/test_native/fakes/`; the real ESP32 implementations are excluded from the `native` build the same way `BluetoothTransport` already is. | PASS |
| V. Safe Motor Control | FR-009 requires the fail-safe `STOP` to bypass the reversal pause and stop/neutral immediately — this is a distinct, directly tested path in `MotorServoVehicleOutput` (going to/from `Stopped` never triggers the pause; only a true forward↔reverse flip does). | PASS |

No unjustified violations. Complexity Tracking table is not needed.

*Re-checked after Phase 1 design (data-model.md, contracts/, quickstart.md): the design introduces
exactly the interfaces/classes anticipated above and nothing else — all five principles still
PASS with no new violations.*

## Project Structure

### Documentation (this feature)

```text
specs/002-motor-servo-actuation/
├── plan.md              # This file (/speckit-plan command output)
├── research.md          # Phase 0 output (/speckit-plan command)
├── data-model.md         # Phase 1 output (/speckit-plan command)
├── quickstart.md         # Phase 1 output (/speckit-plan command)
├── contracts/            # Phase 1 output (/speckit-plan command)
└── tasks.md              # Phase 2 output (/speckit-tasks command - NOT created by /speckit-plan)
```

### Source Code (repository root)

```text
src/
├── hardware/
│   ├── i_vehicle_output.h            # existing; + default no-op tick(nowMs) for periodic hardware updates
│   ├── serial_direction_output.h     # existing, unchanged (inherits tick() default)
│   ├── i_motor_driver.h              # NEW: motor polarity interface (driveForward/driveReverse/stop)
│   ├── gpio_motor_driver.h           # NEW: ESP32 real impl — digitalWrite on config::kMotorPinA/B
│   ├── i_steering_servo.h            # NEW: servo angle interface (setAngleDeg)
│   ├── pwm_steering_servo.h          # NEW: ESP32 real impl — wraps ESP32Servo
│   └── motor_servo_vehicle_output.h  # NEW: IVehicleOutput impl — Direction -> motor + servo,
│                                      #      incl. the non-blocking reversal-pause state machine
│                                      #      (header-only, like the other Hardware-layer classes)
├── config.h                          # + kServoLeftAngleDeg, kServoRightAngleDeg, kMotorReversePauseMs
└── main.cpp                          # bring-up test deleted; wires MotorServoVehicleOutput
                                       # alongside the existing SerialDirectionOutput

test/test_native/
├── fakes/
│   ├── fake_motor_driver.h                 # NEW
│   ├── fake_steering_servo.h                # NEW
│   └── recording_vehicle_output.h           # existing, reused as-is
├── test_motor_servo_vehicle_output.cpp      # NEW: unit tests (mapping + reversal-pause timing)
└── test_command_to_actuation_flow.cpp       # NEW: integration test (fake transport -> ... -> fakes)
```

**Structure Decision**: Single embedded project, unchanged layout. The new pieces slot entirely
into the existing `hardware/` directory and follow the established interface + fake + real-impl
triplet already used for `ITransport`/`IVehicleOutput` (Constitution Principle IV). `main.cpp`
remains the only place concrete implementations are wired together; `Control` and `Protocol` are
untouched.

## Complexity Tracking

*No violations — table intentionally left empty.*
