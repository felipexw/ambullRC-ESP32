# Bluetooth Command Protocol — v1

Satisfies FR-010 (protocol MUST be documented and versioned in the repository).

## Transport

Bluetooth Classic SPP (ESP32 `BluetoothSerial`). Any device that connects may send commands
(FR-011 — no pairing/authorization check in this feature).

## Wire format

One command per line, ASCII, newline (`\n`) terminated. Two forms are accepted, both handled by
`DriveCommandAssembler` (see `specs/002-motor-servo-actuation/`):

### Word commands (what the Android app actually sends)

```
UP | DOWN | LEFT | RIGHT
```

Case-insensitive. Each word updates **only its own axis** — `UP`/`DOWN` set the throttle axis,
`LEFT`/`RIGHT` set the steer axis — the other axis keeps its last known value. This is what makes
e.g. driving forward (`UP`) and then steering right (`RIGHT`) keep the car moving forward while it
turns, instead of one command resetting the other axis to zero:

| Word | Effect |
|------|--------|
| `UP` | `throttle = kThrottleMax` (full forward); `steer` unchanged |
| `DOWN` | `throttle = kThrottleMin` (full reverse); `steer` unchanged |
| `LEFT` | `steer = kSteerMin` (full left); `throttle` unchanged |
| `RIGHT` | `steer = kSteerMax` (full right); `throttle` unchanged |

There is no explicit "stop"/"center" word — releasing a button simply means the app stops sending
that word, and the vehicle returns to a stopped/centered state via the existing fail-safe timeout
(`kCommandTimeoutMs`), which also resets both axes so a stale pre-timeout value can't be
resurrected by the next word command.

### Numeric pairs (manual/terminal testing)

```
<steer>,<throttle>\n
```

- `steer`: integer, `[-100, 100]`. Negative = left, positive = right, `0` = straight.
- `throttle`: integer, `[-100, 100]`. Negative = reverse, positive = forward, `0` = stop.

Unlike a word command, a numeric pair sets **both** axes explicitly (full overwrite) — handy for
sending an exact combined state by hand from a generic SPP terminal app.

### Examples

| Line | Meaning |
|------|---------|
| `UP` | Throttle set to full forward |
| `RIGHT` | Steer set to full right |
| `0,0` | Straight, stopped (both axes explicitly) |
| `0,100` | Straight, full forward (both axes explicitly) |
| `-50,0` | Left, no throttle (both axes explicitly) |
| `40,-30` | Right, reverse (both axes explicitly) |

### Invalid input handling

- A line that is not a recognized word command and does not match `<int>,<int>` → **rejected**,
  discarded, no crash (FR-006). This includes: non-numeric content, a missing field (`10`), extra
  fields (`1,2,3`), and trailing characters after the second integer (`1,2x`). The parser does not
  trim whitespace — a line with surrounding or embedded spaces is treated as malformed.
- A line that parses as numeric but has `steer` or `throttle` outside `[-100, 100]` → **rejected**,
  discarded — not clamped to the boundary (FR-005). This is reported distinctly from a malformed
  line (`ParseResult::OutOfRange` vs. `ParseResult::Malformed` — see `data-model.md`), though both
  are simply discarded from the protocol's point of view.
- Rejected lines do not change the vehicle's current direction, nor either persisted axis; the
  last valid command (or safe state) stands.

## Connection lifecycle logging

Independent of command parsing, every successful connection and every lost connection is logged
to serial exactly once per transition, tagged with the peer's Bluetooth MAC address (no pairing
or name resolution is performed, per FR-011, so the address is the identifier — not a friendly
device name):

| Event | Printed |
|-------|---------|
| A Bluetooth client connects | `BLUETOOTH_CONNECTED AA:BB:CC:DD:EE:FF` |
| The Bluetooth client disconnects | `BLUETOOTH_DISCONNECTED AA:BB:CC:DD:EE:FF` (last known address) |

This is edge-detected (`ConnectionMonitor`) from the transport's `connected()` flag, polled once
per loop iteration — no repeated logging while the state is unchanged. The address is captured
from the SPP server-open event and retained until the next connection overwrites it.

## Current scope (this feature)

Each valid command is translated to a `Direction` (see `data-model.md`) and printed to the serial
console. **No servo or DC-motor output exists in this version of the protocol handling** — that is
introduced by a later feature, reusing this same wire format and `DriveCommand` parsing unchanged.

## Versioning

This is protocol **v1**, extended (non-breaking) by `specs/002-motor-servo-actuation` to also
accept the word-command form above — the original numeric form still parses identically. Future
*breaking* changes to the wire format (field order, added fields, different delimiter, or removing
either accepted form) MUST bump this version and be documented in a new `contracts/` entry rather
than silently changed in place.
