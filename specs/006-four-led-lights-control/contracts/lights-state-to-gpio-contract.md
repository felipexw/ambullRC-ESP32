# LightsState → GPIO Contract

Satisfies FR-001 through FR-003 and FR-006. This is the Control/Hardware boundary: whenever
`LightsControl` produces a new `LightsState`, `GpioLightsOutput` (the real `ILightsOutput`
implementation) MUST drive each light's GPIO pin exactly as follows.

| `LightsState` index | Light | Config pin | `true` (ON) | `false` (OFF) |
|---|---|---|---|---|
| `state[0]` | Light 1 | `config::kLight1Pin` | `HIGH` (3.3V) | `LOW` (0V) |
| `state[1]` | Light 2 | `config::kLight2Pin` | `HIGH` (3.3V) | `LOW` (0V) |
| `state[2]` | Light 3 | `config::kLight3Pin` | `HIGH` (3.3V) | `LOW` (0V) |
| `state[3]` | Light 4 | `config::kLight4Pin` | `HIGH` (3.3V) | `LOW` (0V) |

`apply()` writes all four pins from the full state every call — there is no partial/incremental
form (research.md §7).

## Boot-time default (FR-006)

Before any `LightCommand` is ever applied, all four pins MUST be `LOW` (OFF).
`GpioLightsOutput::begin()` sets all four pins to `OUTPUT` mode and writes `LOW` during `setup()`,
mirroring `LedConnectionOutput::begin()` and `GpioMotorDriver::begin()`.

## Independence from other signals (FR-003)

This contract is driven only by `LightsState` as computed by `LightsControl` from `LightCommand`s.
It is not affected by `ConnectionEvent`, `Direction`, or any motor/servo fail-safe transition — a
disconnect or command timeout that stops the DC motor and centers the servo does not change any
light's GPIO level (spec FR-009 / Assumptions; Constitution Principle V's fail-safe scope is the
DC motor and steering servo only).

## Out of scope

The physical LED circuits (current-limiting resistors, mounting) for the four lights are out of
scope, per the spec's Assumptions — this contract covers only the logic-level GPIO signal.
