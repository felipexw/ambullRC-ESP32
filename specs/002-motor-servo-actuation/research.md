# Phase 0 Research: Motor & Servo Actuation

## 1. Keeping serial logging *and* adding real actuation

**Decision**: Wire both `IVehicleOutput` implementations directly in `main.cpp` — the existing
`SerialDirectionOutput` and the new `MotorServoVehicleOutput` — via a tiny local
`emitDirection(Direction)` helper that calls `emit()` on both. No fan-out/multiplexer abstraction
is introduced.

**Rationale**: There are exactly two call sites in `main.cpp` (the command path and the fail-safe
tick path). A composite `IVehicleOutput` that forwards to a list of outputs would be unjustified
generality for two known, fixed call sites (Principle I). The local helper still guarantees both
outputs never drift out of sync at a new call site.

**Alternatives considered**: A `MultiVehicleOutput` composite implementing `IVehicleOutput` over a
list of delegates — rejected as speculative (no third output is anticipated) and adds an
abstraction layer the constitution says must be justified by a concrete need.

## 2. Non-blocking reversal pause (FR-010)

**Decision**: Split hardware output into an immediate decision (`emit(Direction)`) and a periodic,
time-aware step (`tick(unsigned long nowMs)`). `emit()` records the desired motor polarity and
applies the servo angle immediately (steering has no pause requirement). `tick()`, called once per
`loop()` iteration unconditionally, advances the motor's state machine: on a true forward↔reverse
flip it stops the motor immediately and starts a 300ms pause; once the pause elapses, it engages
the new polarity. Going to/from `Stopped` never triggers the pause (needed so FR-009's fail-safe
stop is immediate).

`tick(unsigned long nowMs)` is added to `IVehicleOutput` as a virtual method with an empty default
body, so `SerialDirectionOutput` needs no change.

**Rationale**: `delay()` cannot be used (the project already fixed a blocking-delay bug — see
commit `0d4a9e9`); the pause must survive across multiple `loop()` iterations without blocking
command processing or the fail-safe tick. This mirrors the exact pattern already used by
`ConnectionMonitor::onTick` and the (now-deleted) hardware bring-up test's own step function, so it
introduces no new pattern to the codebase, just applies the existing one.

**Alternatives considered**: Giving `emit()` a `nowMs` parameter and doing everything inline —
rejected because the pause must keep advancing even if the app briefly stops sending commands
(unlikely in practice, but `tick()` decouples correctness from command cadence, matching how
`ConnectionMonitor` already decouples fail-safe timing from command arrival).

## 3. Motor and servo hardware abstraction

**Decision**: Introduce `IMotorDriver` (`driveForward()` / `driveReverse()` / `stop()`) and
`ISteeringServo` (`setAngleDeg(int)`), each with an ESP32 real implementation
(`GpioMotorDriver` using `digitalWrite` on `config::kMotorPinA`/`kMotorPinB`; `PwmSteeringServo`
wrapping the already-used `ESP32Servo` library) and a host-test fake. `MotorServoVehicleOutput`
depends only on the interfaces.

**Rationale**: Constitution Principle IV requires hardware access behind interfaces with fakes for
tests. The reversal-pause state machine is exactly the kind of logic that must be host-testable
(Principle II) — that requires injecting fakes for the GPIO/servo calls instead of calling
`digitalWrite`/`Servo::write` directly. This follows the same interface + fake + real-impl triplet
already established for `ITransport` (→ `BluetoothTransport` / `FakeTransport`) and `IVehicleOutput`
(→ `SerialDirectionOutput` / `RecordingVehicleOutput`).

**Alternatives considered**: Putting `digitalWrite`/`Servo` calls directly inside
`MotorServoVehicleOutput` — rejected because it would make the reversal-pause logic untestable on
host (violates Principle II/IV) and would pull `Arduino.h` into a class that otherwise contains
pure, testable logic.

## 4. Steering angle application timing

**Decision (superseded — see below)**: The servo snaps directly to its target angle
(`setAngleDeg()`), with no ramping or gradual sweep.

**Rationale**: The deleted bring-up test's slow visual sweep existed purely so a human could watch
the servo move during wiring verification (already done). Real steering should be as responsive as
the servo hardware allows, consistent with SC-002's 250ms target; a real RC car's steering doesn't
ease into position.

**Alternatives considered**: Reusing the bring-up test's `map()`-interpolated slow movement —
rejected: it would make steering feel laggy and has no remaining wiring-verification purpose once
the bring-up test itself is deleted.

**Superseded during on-device validation**: this assumed the servo was positional (holds a
commanded angle while powered, like a normal RC steering servo). On-device testing showed it is
actually continuous-rotation (360°): a non-neutral pulse commands a spin *speed*, not a held
position, so snapping to `kServoLeftAngleDeg`/`kServoRightAngleDeg` and leaving it there spins the
servo forever the instant any turn direction is decided — since nothing else ever tells it to
stop, this can be effectively permanent (the app's per-axis word protocol has no explicit
"release"/"center" command; the axis just latches until a different command changes it). Fixed by
giving the servo a bounded-pulse state machine in `MotorServoVehicleOutput` (mirroring the motor's
`tick()`-driven timing, for a different reason): a turn angle starts a
`config::kServoTurnPulseMs` timer, and once it elapses with no new angle applied, `tick()`
force-writes `kServoNeutralAngleDeg` again — self-stopping regardless of how long the app keeps
sending (or fails to release) `LEFT`/`RIGHT`. Reaching `kServoNeutralAngleDeg` still applies
immediately, per the original decision above.

## 5. Direction → actuation mapping

**Decision**: Documented as the authoritative Control/Hardware boundary contract in
`contracts/direction-to-actuation-contract.md` — one row per `Direction` value, giving the exact
motor polarity and servo angle it must produce.

**Rationale**: `Direction` already fully determines the desired motor+servo state (per spec
FR-001–006); writing this down once as a table removes any ambiguity for both the implementation
and its tests, mirroring how `001-bluetooth-motor-control` documented its wire protocol as a
contract.

**Alternatives considered**: Leaving the mapping implicit in code/tests only — rejected; a written
contract is cheap and gives unit tests a single source of truth to assert against.
