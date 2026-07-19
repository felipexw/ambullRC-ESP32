# Implementation Plan: Bluetooth Motor Control

**Branch**: `001-bluetooth-motor-control` | **Date**: 2026-07-18 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/001-bluetooth-motor-control/spec.md`

## Summary

The ESP32 receives drive commands (steering + throttle) from the Android app over Bluetooth.
**This plan covers a deliberately reduced first slice**, per explicit scoping direction: parse
each received command and print the resulting direction (e.g. `FORWARD`, `LEFT`, `STOP`) to the
serial console — **the DC motor and servo are not driven yet**.

The full `Transport → Protocol → Control → Hardware` layering from the constitution is still
built out end-to-end now. Only the concrete `Hardware` implementation differs: instead of a real
servo/DC-motor driver, this milestone uses a serial-logging stand-in that prints the direction the
Control layer decided on. Protocol parsing, direction-decision logic, and fail-safe behavior are
implemented and tested for real. A later feature swaps in the real motor-driving `Hardware`
implementation behind the same interface — no rework of Transport/Protocol/Control.

## Technical Context

**Language/Version**: C++ (Arduino core for ESP32, C++17 where the toolchain allows), via
PlatformIO

**Primary Dependencies**: `BluetoothSerial` (bundled with the ESP32 Arduino core — no new
third-party dependency) for the Transport layer; PlatformIO Unity for tests. No JSON or other
parsing library — the command format is a minimal hand-parsed text line (see research.md).

**Storage**: N/A (no persisted state; all state is in-memory for the life of a connection)

**Testing**: Unity via `pio test -e native` (existing `test/test_native`), extended with unit
tests for Protocol parsing and Control direction logic, and an integration test that drives a
fake Transport through to the logging Hardware stand-in.

**Target Platform**: ESP32 Dev Module (`esp32dev`, Arduino framework) for firmware; `native` host
environment for tests (Constitution Principle II — no physical ESP32 required for tests).

**Project Type**: Single embedded firmware project (existing PlatformIO layout: `src/`, `test/`).

**Performance Goals**: A received command produces a printed direction within the spec's SC-002
target of 250ms.

**Constraints**: No servo or DC-motor actuation in this milestone (explicit scope reduction — the
`Hardware` layer only logs). Must build/run fully on host for tests, without a physical ESP32.

**Scale/Scope**: Single ESP32, single Bluetooth connection at a time, per spec Assumptions.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Check | Result |
|-----------|-------|--------|
| I. Simplicity First (YAGNI) | Scope is intentionally cut to the smallest useful slice (receive → parse → decide → log); no motor-driving code, pairing/auth, or protocol framework added before needed. | PASS |
| II. Test-First | Plan requires unit tests for command parsing and direction-decision logic, plus an integration test for the full command-in → direction-logged-out path against fakes, before the slice is done. | PASS |
| III. Simple, Layered Architecture | `Transport → Protocol → Control → Hardware` is implemented in full for this slice; no layer is skipped or bypassed. The `Hardware` layer is a real (if minimal) implementation of the vehicle-output interface, not a shortcut around it. | PASS |
| IV. Hardware Abstraction for Testability | Both the Bluetooth transport and the vehicle-output are accessed through interfaces (`ITransport`, `IVehicleOutput`) with fakes used in host tests. The ESP32-only `BluetoothSerial` code is isolated behind `ITransport` so Protocol/Control never touch it directly. | PASS |
| V. Safe Motor Control | No physical motor output exists yet, so physical safety doesn't apply this slice — but the Control layer still computes and tests a defined safe direction (`STOP`) on disconnect, timeout, and malformed commands (FR-007/008/009), so the decision logic is correct and tested before real motor output is wired in. | PASS (logical safe-state only; physical enforcement deferred to the feature that adds real motor `Hardware`) |

No unjustified violations. Complexity Tracking table is not needed.

## Project Structure

### Documentation (this feature)

```text
specs/001-bluetooth-motor-control/
├── plan.md              # This file (/speckit-plan command output)
├── research.md          # Phase 0 output (/speckit-plan command)
├── data-model.md        # Phase 1 output (/speckit-plan command)
├── quickstart.md        # Phase 1 output (/speckit-plan command)
├── contracts/           # Phase 1 output (/speckit-plan command)
└── tasks.md             # Phase 2 output (/speckit-tasks command - NOT created by /speckit-plan)
```

### Source Code (repository root)

```text
src/
├── transport/
│   ├── i_transport.h          # ITransport interface (byte-line in/out)
│   └── bluetooth_transport.h  # ESP32 BluetoothSerial implementation (esp32dev only)
├── protocol/
│   ├── command_parser.h       # Pure: raw line -> DriveCommand (or parse error)
│   └── command_parser.cpp
├── control/
│   ├── direction_control.h    # Pure: DriveCommand + connection state -> Direction
│   └── direction_control.cpp
├── hardware/
│   ├── i_vehicle_output.h     # IVehicleOutput interface (accepts a Direction)
│   └── serial_direction_output.h  # ESP32 impl: Serial.println(direction) — no motor I/O
└── main.cpp                   # Wires Transport -> Protocol -> Control -> Hardware for esp32dev

test/
└── test_native/
    ├── test_dummy.cpp                    # existing toolchain smoke test
    ├── test_command_parser.cpp           # unit: Protocol
    ├── test_direction_control.cpp        # unit: Control (incl. safe-state cases)
    ├── fakes/
    │   ├── fake_transport.h              # ITransport fake for tests
    │   └── recording_vehicle_output.h    # IVehicleOutput fake that records calls
    └── test_command_to_direction_flow.cpp  # integration: fake transport -> ... -> recorded output
```

**Structure Decision**: Single embedded project (matches the existing PlatformIO layout). Source
is split by the constitution's four layers (`transport/`, `protocol/`, `control/`, `hardware/`)
rather than generic `models/services/lib`, so the one-way layering is visible in the directory
structure itself. `main.cpp` is the only place that wires concrete implementations together;
everything else depends on interfaces, keeping Protocol and Control host-testable.

## Complexity Tracking

*No violations — table intentionally left empty.*
