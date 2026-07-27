# Phase 0 Research: Connection Status LED

## 1. What "successful connection" and "connection lost" mean for the LED

**Decision**: The LED is driven exclusively by `ConnectionEvent` as already produced by
`ConnectionMonitor` from the Bluetooth transport's `connected()` signal — `Connected` turns the
LED ON, `Disconnected` turns it OFF. It is **not** driven by `DirectionControl`'s independent
command-timeout safe-state (the mechanism that stops the motor/servo when no valid drive command
arrives within `config::kCommandTimeoutMs`, even while the Bluetooth link itself is still up).

**Rationale**: The spec's Input is explicit that the LED's "purpose is only to give feedback
regarding the Bluetooth connection (paired)" — a link-level concept — and its Assumptions state
"successful Bluetooth connection" reuses the same connection-established condition already
recognized elsewhere in the system (i.e. `ConnectionEvent`, not a new/stricter definition).
Command timeout is a distinct, motor-safety concept: the phone can still be paired and connected
while simply not sending commands (e.g. app briefly backgrounded), and per spec FR-004 the LED
"MUST NOT be used to signal any other condition" beyond the Bluetooth connection itself. Reusing
`ConnectionEvent` unchanged is also the simplest option (Principle I) and requires no new state or
wiring beyond one more `IConnectionOutput` consumer.

**Alternatives considered**: Tying the LED to `DirectionControl`'s combined
disconnected-or-timed-out safe state — rejected because it would make the LED flicker off during
normal brief command gaps that are not a Bluetooth disconnection, contradicting the spec's stated
single purpose and FR-004, and would require threading a second signal (from `Control`) into an
indicator the spec defines purely in terms of link status.

## 2. Interface reuse vs. a new interface

**Decision**: Add `LedConnectionOutput` as a second implementation of the existing
`IConnectionOutput` interface (same interface `SerialConnectionOutput` already implements). No new
interface is introduced.

**Rationale**: `IConnectionOutput::emit(ConnectionEvent, deviceId)` already expresses exactly what
the LED needs to react to, with the same event semantics (called at most once per transition,
`deviceId` available but unused). Two consumers of one event via the same interface is precisely
the "fan out to two Hardware sinks" shape `002-motor-servo-actuation` already established for
`Direction` (`SerialDirectionOutput` + `MotorServoVehicleOutput`). Introducing a distinct
`ILedOutput` would duplicate an interface that already fits, violating Principle I.

**Alternatives considered**: A combined `IConnectionOutput` implementation that both logs to
serial and drives the LED in one class — rejected because it conflates two independent Hardware
concerns (a log line vs. a physical signal) behind one implementation, making each harder to
reason about and test in isolation, and diverging from the established one-concern-per-class
pattern (`SerialDirectionOutput` vs. `MotorServoVehicleOutput`).

## 3. GPIO pin selection

**Decision**: Use a single dedicated GPIO pin (`config::kLedPin`), distinct from the servo pin
(`kServoPin` = 13) and the DC motor driver pins (`kMotorPinA` = 18, `kMotorPinB` = 19), chosen from
the ESP32 Dev Module's general-purpose pins that are not boot-strapping pins (avoiding GPIO0, 2,
5, 12, 15) and not otherwise reserved.

**Rationale**: Avoiding strapping pins prevents the LED's default output level from interfering
with the board's boot mode selection — the same caution already implicit in how `kServoPin`/
`kMotorPinA`/`kMotorPinB` were chosen. The exact pin number is an implementation-time config value
(`src/config.h`), not a spec-level decision; `data-model.md`/`contracts/` reference it symbolically
as `config::kLedPin`.

**Alternatives considered**: The ESP32 Dev Module's onboard LED (commonly GPIO2) — rejected because
it's a strapping pin on most ESP32 modules and the spec explicitly describes an LED "hanging
outside the RC car" (i.e. an external indicator), not the onboard one.

## 4. Signal level semantics (3.3V ON / 0V OFF)

**Decision**: `Connected` → `digitalWrite(kLedPin, HIGH)` (3.3V logic level); `Disconnected` (and
the boot-time default, before any connection) → `digitalWrite(kLedPin, LOW)` (0V).

**Rationale**: Directly matches the spec Input's explicit instruction ("consider sending a signal
3.3V to this LED") and FR-001/002/003. `HIGH` on the ESP32's GPIO output is 3.3V, matching the
literal ask; the physical LED/current-limiting circuit that turns this logic signal into visible
light is explicitly out of scope (spec Input, Key Entities).

**Alternatives considered**: Active-low wiring (LOW = ON) — rejected as an unnecessary inversion
with no stated requirement for it; the spec's plain-language "switch on"/"switch off" maps most
directly to a simple active-high digital signal.
