# Bluetooth Command Protocol — v1

Satisfies FR-010 (protocol MUST be documented and versioned in the repository).

## Transport

Bluetooth Classic SPP (ESP32 `BluetoothSerial`). Any device that connects may send commands
(FR-011 — no pairing/authorization check in this feature).

## Wire format

One command per line, ASCII, newline (`\n`) terminated:

```
<steer>,<throttle>\n
```

- `steer`: integer, `[-100, 100]`. Negative = left, positive = right, `0` = straight.
- `throttle`: integer, `[-100, 100]`. Negative = reverse, positive = forward, `0` = stop.

### Examples

| Line | Meaning |
|------|---------|
| `0,0` | Straight, stopped |
| `0,100` | Straight, full forward |
| `-50,0` | Left, no throttle |
| `40,-30` | Right, reverse |

### Invalid input handling

- A line that does not match `<int>,<int>` → **rejected**, discarded, no crash (FR-006). This
  includes: non-numeric content, a missing field (`10`), extra fields (`1,2,3`), and trailing
  characters after the second integer (`1,2x`). The parser does not trim whitespace — a line with
  surrounding or embedded spaces is treated as malformed.
- A line that parses but has `steer` or `throttle` outside `[-100, 100]` → **rejected**, discarded
  — not clamped to the boundary (FR-005). This is reported distinctly from a malformed line
  (`ParseResult::OutOfRange` vs. `ParseResult::Malformed` — see `data-model.md`), though both are
  simply discarded from the protocol's point of view.
- Rejected lines do not change the vehicle's current direction; the last valid command (or safe
  state) stands.

## Connection lifecycle logging

Independent of command parsing, every successful connection and every lost connection is logged
to serial exactly once per transition:

| Event | Printed |
|-------|---------|
| A Bluetooth client connects | `BLUETOOTH_CONNECTED` |
| The Bluetooth client disconnects | `BLUETOOTH_DISCONNECTED` |

This is edge-detected (`ConnectionMonitor`) from the transport's `connected()` flag, polled once
per loop iteration — no repeated logging while the state is unchanged.

## Current scope (this feature)

Each valid command is translated to a `Direction` (see `data-model.md`) and printed to the serial
console. **No servo or DC-motor output exists in this version of the protocol handling** — that is
introduced by a later feature, reusing this same wire format and `DriveCommand` parsing unchanged.

## Versioning

This is protocol **v1**. Future breaking changes to the wire format (field order, added fields,
different delimiter) MUST bump this version and be documented in a new `contracts/` entry rather
than silently changed in place.
