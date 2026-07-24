# Direction → Actuation Contract

Satisfies FR-001 through FR-006. This is the authoritative Control/Hardware boundary: for every
`Direction` the `Control` layer can produce, `MotorServoVehicleOutput` MUST drive the motor and
servo exactly as follows. No other `Direction` values exist.

| Direction | Motor polarity | Servo angle |
|-----------|-----------------|-------------|
| `STOP` | Stopped | `kServoNeutralAngleDeg` |
| `FORWARD` | Forward | `kServoNeutralAngleDeg` |
| `BACKWARD` | Reverse | `kServoNeutralAngleDeg` |
| `LEFT` | Stopped | `kServoLeftAngleDeg` |
| `RIGHT` | Stopped | `kServoRightAngleDeg` |
| `FORWARD_LEFT` | Forward | `kServoLeftAngleDeg` |
| `FORWARD_RIGHT` | Forward | `kServoRightAngleDeg` |
| `BACKWARD_LEFT` | Reverse | `kServoLeftAngleDeg` |
| `BACKWARD_RIGHT` | Reverse | `kServoRightAngleDeg` |

## Timing rules

- **Servo angle**: the physical servo is continuous-rotation (360°), not positional — it has no
  way to hold a fixed angle, only a speed/direction set by the commanded pulse. A `LEFT`/`RIGHT`
  angle is therefore applied as a bounded pulse (`config::kServoTurnPulseMs`, applied via `tick()`)
  long enough to swing the steering linkage to its lock; once the pulse elapses, the servo
  auto-reverts to `kServoNeutralAngleDeg` regardless of whether the `Direction` is still
  `LEFT`/`RIGHT`. Without this, the servo would spin forever once any turn direction is decided,
  since nothing else would ever tell it to stop (see `research.md` §4, updated).
- **Motor polarity**: applied immediately *unless* the change is a true reversal (`Forward` ↔
  `Reverse` with neither side `Stopped`), per `data-model.md`'s state machine. A true reversal
  first stops the motor, then waits `config::kMotorReversePauseMs` (300ms) before engaging the new
  polarity (FR-010).
- **Fail-safe** (`STOP` reached via disconnect, timeout, or ignored malformed command — see
  `specs/001-bluetooth-motor-control/data-model.md`): always centers the servo and stops the motor
  immediately, cancelling any in-progress turn pulse — never gated behind either timer, because
  `Stopped`/`kServoNeutralAngleDeg` is always the target of that transition (FR-009).

## Independence (FR-007)

Motor polarity and servo angle are derived independently from the same `Direction` value and
applied independently — e.g. reaching `FORWARD_RIGHT` while already mid-reversal-pause still
updates the servo to `kServoRightAngleDeg` immediately, even though the motor is still waiting out
its pause.

## Out of scope

This contract does not change `001-bluetooth-motor-control`'s wire protocol
(`contracts/bluetooth-command-protocol.md`) or `Direction` decision logic — both are reused
unchanged as the input to this contract.
