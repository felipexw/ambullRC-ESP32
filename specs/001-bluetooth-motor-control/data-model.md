# Data Model: Bluetooth Motor Control (direction-print slice)

## DriveCommand

Parsed result of one received protocol line (Protocol layer output).

| Field | Type | Constraints |
|-------|------|-------------|
| `steer` | integer | `[-100, 100]`; negative = left, positive = right, 0 = straight |
| `throttle` | integer | `[-100, 100]`; negative = reverse, positive = forward, 0 = stop |

A line that fails to parse, or parses with either field out of `[-100, 100]`, produces no
`DriveCommand` — it is rejected (FR-005, FR-006) and never reaches the Control layer.

## Direction

Enum produced by the Control layer from a `DriveCommand` or a safe-state trigger. This is the
*only* thing printed in this slice — no motor/servo values.

```
STOP | FORWARD | BACKWARD | LEFT | RIGHT |
FORWARD_LEFT | FORWARD_RIGHT | BACKWARD_LEFT | BACKWARD_RIGHT
```

Derivation from a valid `DriveCommand`:

| throttle | steer | Direction |
|----------|-------|-----------|
| 0 | 0 | `STOP` |
| > 0 | 0 | `FORWARD` |
| < 0 | 0 | `BACKWARD` |
| 0 | < 0 | `LEFT` |
| 0 | > 0 | `RIGHT` |
| > 0 | < 0 | `FORWARD_LEFT` |
| > 0 | > 0 | `FORWARD_RIGHT` |
| < 0 | < 0 | `BACKWARD_LEFT` |
| < 0 | > 0 | `BACKWARD_RIGHT` |

## ConnectionState

Tracked by the Control layer (fed by the Transport layer through the main loop), used to decide
when to emit a safe-state `Direction`.

| Field | Type | Notes |
|-------|------|-------|
| `connected` | boolean | From `ITransport` |
| `lastCommandAtMs` | integer (millis) | Updated on each valid `DriveCommand`; compared against the configured timeout to detect a stale connection |

### State Transitions

```
DISCONNECTED / TIMED_OUT / MALFORMED-triggered-safe-state
        │  (new valid DriveCommand received)
        ▼
     ACTIVE (direction reflects latest valid DriveCommand)
        │  (disconnect OR no valid command within timeout)
        ▼
   SAFE (direction = STOP, emitted once on transition)
```

Malformed/out-of-range commands do not themselves cause a state transition (FR-005/FR-006) — they
are simply ignored; only a disconnect or timeout drives `ACTIVE → SAFE`.
