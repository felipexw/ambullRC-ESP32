# Phase 0 Research: Bluetooth Motor Control (direction-print slice)

## 1. Bluetooth transport: Classic SPP vs BLE

**Decision**: Use Bluetooth Classic SPP via the ESP32 Arduino core's built-in `BluetoothSerial`
library, exposed as a stream of newline-terminated text lines.

**Rationale**: `BluetoothSerial` ships with the `framework = arduino` toolchain already declared
in `platformio.ini` — no new dependency, matching the constitution's "prefer the standard toolchain
already in use" constraint and Principle I (YAGNI). It presents a simple `Stream`-like read/write
API that maps directly onto a minimal `ITransport` interface (read a line, know if connected).
Any terminal-style Android BLE/SPP app, or a small custom app, can drive it.

**Alternatives considered**: BLE (GATT services/characteristics) — rejected for this slice: it
requires defining services/characteristics/UUIDs and a more involved connection-state model,
which is unjustified complexity before the simplest possible command path is proven. Revisit if
the target Android app can only use BLE.

## 2. Command wire format

**Decision**: A single ASCII line per command, newline-terminated:

```
<steer>,<throttle>\n
```

- `steer`: integer in `[-100, 100]`. Negative = left, positive = right, 0 = straight.
- `throttle`: integer in `[-100, 100]`. Negative = reverse, positive = forward, 0 = stop.

Documented and versioned as **protocol v1** in `contracts/bluetooth-command-protocol.md`.

**Rationale**: Satisfies FR-010 (documented, versioned protocol) with the smallest possible
parser — no JSON/CBOR library needed, easy to type by hand from a Bluetooth terminal app for
manual testing, and trivially fuzzable for the malformed-input tests required by FR-006/US2.

**Alternatives considered**: JSON per command — rejected as unjustified weight (new dependency,
slower to parse) for two integers. A binary/fixed-byte protocol — rejected as harder to test
manually via a generic Bluetooth terminal app during this early slice; can be revisited later if
throughput becomes a real constraint (none identified yet).

## 3. Direction vocabulary (Control layer output)

**Decision**: `Direction` is one of: `STOP`, `FORWARD`, `BACKWARD`, `LEFT`, `RIGHT`,
`FORWARD_LEFT`, `FORWARD_RIGHT`, `BACKWARD_LEFT`, `BACKWARD_RIGHT`. Derived purely from the sign
of `throttle` (forward/backward/none) combined with the sign of `steer` (left/right/none); both
zero maps to `STOP`.

**Rationale**: Directly answers the scoped request ("just print the direction") with a small,
exhaustive, easily-tested enum. No magnitude information is discarded silently — it simply isn't
part of this slice's output, consistent with "don't move the motors for now."

**Alternatives considered**: Printing raw `(steer, throttle)` pairs — rejected because the ask was
specifically for *direction*, and a named enum is what a future `Hardware` (real motor) layer will
also need to consume.

## 4. Fail-safe direction on disconnect / timeout / malformed input

**Decision**: On Bluetooth disconnect or command timeout, the Control layer emits `STOP` exactly
once per transition into the safe state (not repeatedly every loop iteration). On a malformed or
out-of-range command, nothing is emitted for that command (FR-005/FR-006) and the last known
direction stands until a new valid command or a safe-state trigger changes it.

**Rationale**: Matches FR-007/008/009 (safe-state on disconnect/timeout, auto-resume on next valid
command) at the decision-logic level, so the logic is correct and tested now even though no motor
is physically stopped yet. Emitting once (not per loop) keeps the serial log readable and is
simple to assert on in tests.

**Alternatives considered**: Re-printing `STOP` continuously while disconnected — rejected as
noisy and not needed to prove the logic; the "once per transition" behavior is what a future motor
`Hardware` layer would want anyway (avoid redundant motor writes).

## 5. Testability of the ESP32-only Transport code

**Decision**: `BluetoothTransport` (the concrete `BluetoothSerial`-based class) lives in its own
header, included only by `main.cpp` for the `esp32dev` build. `test/test_native` never compiles it
— `pio test -e native` builds `protocol/`, `control/`, and a `FakeTransport`/
`RecordingVehicleOutput` test double instead, per the existing `native` env's role (Constitution
Principle II).

**Rationale**: Keeps the host test build free of `Arduino.h`/`BluetoothSerial.h`, avoiding the
exact "file not found" failure already seen when `native` tried to compile Arduino-only code.

**Alternatives considered**: `#ifdef ARDUINO` guards inside a single file — rejected as more
error-prone than simply not referencing ESP32-only headers from any file the `native` env
compiles.
