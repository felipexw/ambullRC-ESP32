# Connection Event → LED Signal Contract

Satisfies FR-001 through FR-004. This is the authoritative Control/Hardware boundary: for every
`ConnectionEvent` the `Control` layer can produce, `LedConnectionOutput` MUST drive the LED's GPIO
signal exactly as follows. No other `ConnectionEvent` value reaches this consumer.

| ConnectionEvent | LED GPIO signal | Meaning |
|------------------|------------------|---------|
| `Connected` | `HIGH` (3.3V) | LED ON — Bluetooth link with the app is active. |
| `Disconnected` | `LOW` (0V) | LED OFF — Bluetooth link is not active. |
| `None` | (not called) | No change — `main.cpp` only dispatches on an actual transition. |

## Boot-time default (FR-003)

Before the first `ConnectionEvent::Connected` is ever observed, the LED signal MUST be `LOW`
(OFF). `LedConnectionOutput::begin()` sets the pin to `OUTPUT` mode and writes `LOW` during
`setup()`, so the LED is never left in an undefined or floating state at power-on.

## Independence from other fail-safe behavior (FR-004, research.md §1)

This contract is driven **only** by `ConnectionEvent` (the Bluetooth link's own connect/disconnect
transitions). It is intentionally **not** driven by `DirectionControl`'s command-timeout safe-state
(`specs/001-bluetooth-motor-control`'s FR-007/008/009, extended in
`specs/003-direction-interlock-guard`) — that mechanism stops the motor/servo when commands go
stale, which can happen while the Bluetooth link itself is still connected. The LED continues to
report `ON` in that case, since the link is not actually lost. See `research.md` §1 for the
rationale.

## No intermediate states

There is no "connecting" or "pairing in progress" signal — `ConnectionEvent` itself has no such
state (only `None`/`Connected`/`Disconnected`), so the LED has exactly two levels and never
blinks, pulses, or dims (per spec Assumptions).

## Out of scope

This contract does not change `ConnectionEvent`, `ConnectionMonitor`, or the existing
`SerialConnectionOutput`/`IConnectionOutput` contract — it is reused unchanged as the input to this
contract. The physical LED circuit (current-limiting resistor, mounting, wiring outside the car) is
out of scope, per the spec Input and Key Entities.
