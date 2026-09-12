# Implementation Plan: Four-Light Auxiliary Control

**Branch**: `006-four-led-lights-control` | **Date**: 2026-09-11 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/006-four-led-lights-control/spec.md`

## Summary

Adds four independently switchable auxiliary lights, each driven by its own GPIO pin, controlled
by eight new word commands (`LIGHT<n>ON`/`LIGHT<n>OFF`, `n` = 1–4) added to the existing Bluetooth
wire protocol, alongside a new capability the protocol has never needed before: the ESP32 sends
data back to the app. A new `LightsControl` (Control layer) tracks the four lights' ON/OFF state
from validated `LightCommand`s; a new `GpioLightsOutput` (Hardware layer, behind a new
`ILightsOutput` interface) drives the four pins from that state; and a new `writeLine()` method on
`ITransport` lets `main.cpp` push a single combined state report (`LIGHTS:bbbb`) back to the app
whenever a light's state actually changes or a new connection is established. This follows the
same "Protocol parses → Control decides → Hardware acts, main.cpp wires it together" shape already
used by `001`/`002`/`004`, extended by exactly one new outbound primitive on the Transport layer.

## Technical Context

**Language/Version**: C++ (Arduino core for ESP32, C++17 where the toolchain allows), via
PlatformIO — same as `001`/`002`/`004`.

**Primary Dependencies**: None new. Plain Arduino `pinMode`/`digitalWrite` for the four new
outputs (same mechanism as `GpioMotorDriver`/`LedConnectionOutput`); `BluetoothSerial::println()`
for the one new outbound line (the same library already used for `readLine()`). PlatformIO Unity
for tests.

**Storage**: N/A (no persisted state — `LightsState` starts all-OFF every boot and is re-derived
only from commands received after boot, per spec FR-006/Assumptions).

**Testing**: Unity via `pio test -e native`. New host-testable logic: `parseLightCommand()`
(Protocol), `formatLightsReport()` (Protocol), `LightsControl` (Control) — all pure functions/pure
logic with no Arduino dependency, so all three get full unit tests, plus one integration test
(`test_light_command_to_report_flow.cpp`) exercising the received-line → applied-state →
written-report path against `FakeTransport` and a new `RecordingLightsOutput` fake.
`GpioLightsOutput` itself is a thin ESP32-only GPIO wrapper (like `GpioMotorDriver`/
`LedConnectionOutput`) and is validated on-device via `quickstart.md`, not a host unit test — same
precedent as `004-connection-status-led`.

**Target Platform**: ESP32 Dev Module (`esp32dev`, Arduino framework) for firmware; `native` host
environment for tests (Constitution Principle II).

**Project Type**: Single embedded firmware project (existing PlatformIO layout: `src/`, `test/`).

**Performance Goals**: Per spec SC-001/SC-002/SC-003, a light's physical state and the app's
`LIGHTS:` report both reflect a command or a new connection within the same loop iteration the
triggering event is detected in — no perceptible delay, matching the existing connection-LED and
drive-command timing.

**Constraints**: Non-breaking extension of the existing wire protocol only (research.md §1/§2) —
no existing word, numeric form, or behavior changes shape. Four independent ON/OFF-only outputs,
no dimming/blinking/color (per spec Assumptions). Physical LED circuits, current-limiting, and
mounting are out of scope (per spec Assumptions). Lights are explicitly outside the Constitution's
motor/servo fail-safe scope (Principle V) — a disconnect or command timeout MUST NOT change any
light's state (spec FR-009).

**Scale/Scope**: Single ESP32, four independent light outputs, one active Bluetooth connection at
a time, per spec Assumptions.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Check | Result |
|-----------|-------|--------|
| I. Simplicity First (YAGNI) | One new Control class (`LightsControl`), one new Hardware interface + impl (`ILightsOutput`/`GpioLightsOutput`), two small Protocol additions (`parseLightCommand`/`formatLightsReport`), and one new `ITransport` method (`writeLine`) — no new framework, no speculative multi-transport or multi-light-group abstraction. A single combined `LightsState` report (research.md §2) was chosen over four independent report lines specifically to avoid extra surface. | PASS |
| II. Test-First | `parseLightCommand`, `formatLightsReport`, and `LightsControl` are all pure logic and get full unit tests before being wired into `main.cpp`; the received-line-to-report path gets an integration test against fakes (`FakeTransport`, `RecordingLightsOutput`). `GpioLightsOutput` is validated on-device per existing precedent (`004`), not a gap — see Testing above. | PASS |
| III. Simple, Layered Architecture | `Protocol` (parse/format) → `Control` (`LightsControl` decides state) → `Hardware` (`GpioLightsOutput` acts), one-way, exactly like the existing `Direction`/`ConnectionEvent` flows. The one new thing — sending a report back to the app — is added to `Transport` (`ITransport::writeLine`), and `main.cpp` (the existing wiring hub, already the single place that reads every layer's output) is the only code that calls it, the same way it already calls into `Hardware` from a decided `Control` value. No layer gains a new *upward* dependency. | PASS |
| IV. Hardware Abstraction for Testability | `GpioLightsOutput` sits behind a new `ILightsOutput` interface with `RecordingLightsOutput` as its test double, matching the `IMotorDriver`/`ISteeringServo`/`IConnectionOutput` pattern. `ITransport`'s new `writeLine()` is faked by `FakeTransport` (already the test double for the rest of the interface). | PASS |
| V. Safe Motor Control | Not applicable — this feature adds no motor/servo control and explicitly does not extend the motor/servo fail-safe scope to lights (spec FR-009, research.md, Assumptions): a disconnect or command timeout continues to stop the DC motor and center the servo exactly as before, unchanged, and does not touch any light. | PASS (N/A to motor control; scope deliberately excluded per spec) |

No unjustified violations. Complexity Tracking table is not needed.

*Re-checked after Phase 1 design (data-model.md, contracts/, quickstart.md): the design introduces
exactly the pieces anticipated above — two Protocol additions, one Control class, one Hardware
interface + impl, one Transport method — and nothing else. All five principles still PASS with no
new violations.*

## Project Structure

### Documentation (this feature)

```text
specs/006-four-led-lights-control/
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
├── config.h                          # + kLightCount, kLight1Pin..kLight4Pin
├── transport/
│   ├── i_transport.h                 # + virtual writeLine(const std::string&)
│   └── bluetooth_transport.h         # + writeLine() via BluetoothSerial::println()
├── protocol/
│   ├── light_command_parser.h/.cpp   # NEW: LightCommand, LightsState, parseLightCommand()
│   └── lights_report_formatter.h/.cpp # NEW: formatLightsReport(LightsState) -> "LIGHTS:bbbb"
├── control/
│   └── lights_control.h/.cpp         # NEW: LightsControl — per-light state transitions
├── hardware/
│   ├── i_lights_output.h             # NEW: ILightsOutput::apply(const LightsState&)
│   └── gpio_lights_output.h          # NEW: ESP32 real impl — begin() + apply() over 4 pins
└── main.cpp                          # + LightsControl/GpioLightsOutput instances; loop() tries
                                       #   parseLightCommand() before the existing
                                       #   DriveCommandAssembler path; on light-state change or
                                       #   ConnectionEvent::Connected, writes formatLightsReport()
                                       #   via transport.writeLine()

test/test_native/
├── fakes/
│   ├── fake_transport.h                    # + writeLine() recording (existing file, extended)
│   └── recording_lights_output.h           # NEW: ILightsOutput test double
├── test_light_command_parser.cpp           # NEW
├── test_lights_report_formatter.cpp        # NEW
├── test_lights_control.cpp                 # NEW
└── test_light_command_to_report_flow.cpp   # NEW: integration, FakeTransport + RecordingLightsOutput
```

**Structure Decision**: Single embedded project, unchanged layout. Every new piece slots into the
existing `protocol/`, `control/`, `hardware/` directories following the same interface-plus-real-
implementation pattern already established (`IMotorDriver`/`GpioMotorDriver`,
`IConnectionOutput`/`LedConnectionOutput`). The one structural addition outside that pattern is
`ITransport::writeLine()` — a new capability on an existing interface, not a new layer or a new
interface (research.md §4). `main.cpp` remains the only place concrete implementations are wired
together and the only place a `Control`-decided value is handed to `Transport`, mirroring how it
already hands decided values to `Hardware`.

## Complexity Tracking

*No violations — table intentionally left empty.*
