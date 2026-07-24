# Data Model: Motor & Servo Actuation

## Direction (reused, unchanged)

Produced by the existing `Control` layer (`specs/001-bluetooth-motor-control/data-model.md`) —
this feature adds no new values and no new producer of `Direction`:

```
STOP | FORWARD | BACKWARD | LEFT | RIGHT |
FORWARD_LEFT | FORWARD_RIGHT | BACKWARD_LEFT | BACKWARD_RIGHT
```

## MotorPolarity (new, internal to the Hardware layer)

The DC motor's electrical state, as understood by `MotorServoVehicleOutput` and `IMotorDriver`.
Never exposed outside the Hardware layer — `Control` and `Protocol` have no knowledge of it.

```
Stopped | Forward | Reverse
```

Derived from `Direction` per `contracts/direction-to-actuation-contract.md`.

## MotorServoVehicleOutput state (internal)

Tracked entirely inside the new `Hardware`-layer class; not persisted, not exposed.

| Field | Type | Notes |
|-------|------|-------|
| `desiredPolarity` | `MotorPolarity` | Set by the latest `emit(Direction)` call. |
| `appliedPolarity` | `MotorPolarity` | What has actually been written to `IMotorDriver` so far. |
| `pausing` | boolean | `true` while waiting out the 300ms protective pause before engaging a reversed polarity. |
| `pauseStartedAtMs` | integer (millis) | Set when a true forward↔reverse flip begins; compared against `config::kMotorReversePauseMs` in `tick()`. |

### State Transitions

```
appliedPolarity == desiredPolarity
        │ (emit() sets a new desiredPolarity)
        ▼
 desiredPolarity != appliedPolarity
        │
        ├─ appliedPolarity == Stopped OR desiredPolarity == Stopped
        │       → apply immediately (tick()): appliedPolarity = desiredPolarity  [FR-003, FR-009]
        │
        └─ true reversal (Stopped is neither side)
                → motor.stop(); appliedPolarity = Stopped; pausing = true;
                  pauseStartedAtMs = nowMs                                       [FR-010]
                        │ (tick() called on later loop iterations)
                        ▼
                  nowMs - pauseStartedAtMs >= kMotorReversePauseMs
                        │
                        ▼
                  apply desiredPolarity; appliedPolarity = desiredPolarity;
                  pausing = false
```

While `pausing` is `true`, further `emit()` calls may still update `desiredPolarity` (e.g. the
operator lets off the reverse command back to `Stopped` mid-pause); `tick()` always acts on the
*current* `desiredPolarity` once the pause elapses (or immediately, if the new desired state is
`Stopped`, which is not subject to the pause — see FR-009).

The servo angle has its own bounded-pulse state machine, mirroring the motor's shape but for a
different reason: the physical servo is continuous-rotation, so it cannot hold a `LEFT`/`RIGHT`
angle — holding the pulse just spins it forever. `emit()` records the desired angle;
`tick()` applies it (starting a `config::kServoTurnPulseMs` timer whenever the applied angle is
non-neutral) and, once that timer elapses with no new angle having arrived, force-applies
`kServoNeutralAngleDeg` again. Reaching `kServoNeutralAngleDeg` (including via the fail-safe)
always applies immediately and cancels any in-progress pulse. See
`contracts/direction-to-actuation-contract.md`.

| Field | Type | Notes |
|-------|------|-------|
| `desiredServoAngle` | int | Set by the latest `emit(Direction)` call. |
| `appliedServoAngle` | int | What has actually been written to `ISteeringServo` so far. |
| `steeringPulseActive` | boolean | `true` while a non-neutral angle's bounded pulse hasn't yet elapsed. |
| `steerPulseStartedAtMs` | integer (millis) | Set whenever a new angle is applied; compared against `config::kServoTurnPulseMs` in `tick()`. |

## Vehicle State (extends `001-bluetooth-motor-control`'s entity)

| Field | Type | Notes |
|-------|------|-------|
| `direction` | `Direction` | Unchanged — the decided direction, still logged via `SerialDirectionOutput`. |
| `motorPolarity` | `MotorPolarity` | New: the DC motor's actual (applied) state, per above. |
| `servoAngleDeg` | integer | New: the servo's actual angle — one of `kServoLeftAngleDeg`, `kServoNeutralAngleDeg`, `kServoRightAngleDeg`. |
