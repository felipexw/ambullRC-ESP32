# Implementation Plan: Connection Status LED

**Branch**: `004-connection-status-led` | **Date**: 2026-07-27 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/004-connection-status-led/spec.md`

## Summary

The `Control` layer already produces a `ConnectionEvent` (`Connected`/`Disconnected`) each time
`ConnectionMonitor` observes the Bluetooth transport's connection state change, and `main.cpp`
already routes that event to `SerialConnectionOutput` (an `IConnectionOutput` implementation) for
logging. This feature adds a second `Hardware`-layer implementation of the same
`IConnectionOutput` interface, `LedConnectionOutput`, that drives a single GPIO pin HIGH
(3.3V, ON) on `Connected` and LOW (0V, OFF) on `Disconnected`, wired in `main.cpp` **alongside**
(not instead of) the existing serial logging — the same "fan out to two Hardware sinks from one
decided event" shape `002-motor-servo-actuation` already used for `Direction`. No new interface,
no changes to `Transport`/`Protocol`/`Control`, and no change to the wire protocol.

## Technical Context

**Language/Version**: C++ (Arduino core for ESP32, C++17 where the toolchain allows), via
PlatformIO — same as `001`/`002`/`003`.

**Primary Dependencies**: None new. Plain Arduino `pinMode`/`digitalWrite` on a GPIO pin (same
mechanism already used by `GpioMotorDriver`). PlatformIO Unity for tests.

**Storage**: N/A (no persisted state; LED state is always re-derived from live connection status
on every boot, per spec Assumptions).

**Testing**: Unity via `pio test -e native`. The event-routing contract this feature depends on
(`ConnectionMonitor` emits `Connected`/`Disconnected` exactly once per transition, `None`
otherwise) is already covered by the existing `test_connection_monitor.cpp` and
`test_connection_logging_flow.cpp` against the `IConnectionOutput` interface. `LedConnectionOutput`
itself is a thin ESP32-only GPIO wrapper (like `GpioMotorDriver`/`PwmSteeringServo`) and is
validated on-device via `quickstart.md`, not with a host unit test — see Constitution Check
Principle II below for why that's consistent with existing precedent rather than a gap.

**Target Platform**: ESP32 Dev Module (`esp32dev`, Arduino framework) for firmware; `native` host
environment for tests (Constitution Principle II).

**Project Type**: Single embedded firmware project (existing PlatformIO layout: `src/`, `test/`).

**Performance Goals**: Per spec SC-001/SC-002, the LED reflects a connection/disconnection event
within the same loop iteration the event is detected in — no perceptible delay, matching the
existing serial log's timing.

**Constraints**: No new wire protocol or command vocabulary. Single digital ON/OFF GPIO signal
only — no PWM, blinking, or multi-color output (per spec Assumptions). Physical LED circuit,
current-limiting, and mounting are explicitly out of scope (per spec Input). The LED MUST NOT be
driven by anything other than the existing `ConnectionEvent` (FR-004) — in particular, it does
**not** react to `DirectionControl`'s independent command-timeout safe-state (see research.md
Decision 1) since that is a motor/servo safety concern, not a Bluetooth-link concern.

**Scale/Scope**: Single ESP32, single LED output signal, mirroring the single active Bluetooth
connection, per spec Assumptions.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Check | Result |
|-----------|-------|--------|
| I. Simplicity First (YAGNI) | Reuses the existing `IConnectionOutput` interface exactly as-is — no new interface, no new event type. The only additions are one new class (`LedConnectionOutput`) and one config constant (`kLedPin`). | PASS |
| II. Test-First | The logic this feature depends on (`ConnectionEvent` produced exactly once per transition) is already tested by `test_connection_monitor.cpp`/`test_connection_logging_flow.cpp`. `LedConnectionOutput` adds no new host-testable decision logic — it's a direct `ConnectionEvent → digitalWrite` mapping, the same shape as `GpioMotorDriver`/`PwmSteeringServo`, which are likewise validated on-device rather than via host unit tests (see `002-motor-servo-actuation/plan.md`). This is established precedent, not a new exception. | PASS |
| III. Simple, Layered Architecture | Only the `Hardware` layer changes. `Control` still only ever produces a `ConnectionEvent`; it has no knowledge of the LED, its pin, or its signal level. | PASS |
| IV. Hardware Abstraction for Testability | `LedConnectionOutput` sits behind the existing `IConnectionOutput` interface, exactly like `SerialConnectionOutput`. The existing `RecordingConnectionOutput` fake already proves the event-routing logic any `IConnectionOutput` consumer (including this one) relies on. | PASS |
| V. Safe Motor Control | Not directly applicable — this feature adds no motor/servo control. The LED's default-OFF-at-boot and OFF-on-disconnect behavior (FR-002/FR-003) mirrors the same fail-safe spirit the constitution requires elsewhere: the indicator can never claim "connected" when it isn't. | PASS (N/A to motor control; consistent by analogy) |

No unjustified violations. Complexity Tracking table is not needed.

*Re-checked after Phase 1 design (data-model.md, contracts/, quickstart.md): the design introduces
exactly the one class and one config constant anticipated above and nothing else — all five
principles still PASS with no new violations.*

## Project Structure

### Documentation (this feature)

```text
specs/004-connection-status-led/
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
│   ├── i_connection_output.h        # existing, unchanged — reused as-is
│   ├── serial_connection_output.h   # existing, unchanged
│   └── led_connection_output.h      # NEW: ESP32 real impl of IConnectionOutput —
│                                     #      Connected -> digitalWrite(kLedPin, HIGH),
│                                     #      Disconnected -> digitalWrite(kLedPin, LOW),
│                                     #      begin() sets pinMode + defaults to LOW
├── config.h                         # + kLedPin
└── main.cpp                         # + LedConnectionOutput instance, ledOutput.begin() in
                                      #   setup(), and ledOutput.emit(...) alongside the existing
                                      #   connectionOutput.emit(...) in the loop's connection-event
                                      #   dispatch (mirrors the existing emitDirection() fan-out
                                      #   pattern from 002-motor-servo-actuation)

test/test_native/
├── fakes/
│   └── recording_connection_output.h  # existing, reused as-is (no change needed)
├── test_connection_monitor.cpp        # existing, unchanged — already covers the event logic
└── test_connection_logging_flow.cpp   # existing, unchanged — already covers the
                                        # ConnectionMonitor -> IConnectionOutput contract that
                                        # LedConnectionOutput conforms to
```

**Structure Decision**: Single embedded project, unchanged layout. The new piece slots entirely
into the existing `hardware/` directory and follows the established interface + real-impl pattern
already used for `IConnectionOutput` (`SerialConnectionOutput`) and `IVehicleOutput`
(`MotorServoVehicleOutput`) — Constitution Principle IV. `main.cpp` remains the only place
concrete implementations are wired together; `Control`, `Protocol`, and `Transport` are untouched.
No new test doubles or interfaces are needed because this feature adds a second consumer of an
event contract that is already fully specified and tested.

## Complexity Tracking

*No violations — table intentionally left empty.*
