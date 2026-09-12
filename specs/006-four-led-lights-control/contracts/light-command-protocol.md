# Light Command Protocol — extends Bluetooth Command Protocol v1

Satisfies FR-001 through FR-005. Extends (non-breaking)
`specs/001-bluetooth-motor-control/contracts/bluetooth-command-protocol.md` with commands for the
four auxiliary lights. Every existing drive word and numeric pair still parses identically — see
`research.md` §3 for how a line is routed to this parser vs. the existing
`DriveCommandAssembler`.

## Incoming: switching a light on/off

One command per line, ASCII, newline (`\n`) terminated, case-insensitive:

```
LIGHT<n>ON | LIGHT<n>OFF
```

where `<n>` is a single digit `1`–`4` identifying which light.

| Line | Meaning |
|------|---------|
| `LIGHT1ON` | Switch light 1 ON |
| `LIGHT1OFF` | Switch light 1 OFF |
| `LIGHT2ON` | Switch light 2 ON |
| `LIGHT2OFF` | Switch light 2 OFF |
| `LIGHT3ON` | Switch light 3 ON |
| `LIGHT3OFF` | Switch light 3 OFF |
| `LIGHT4ON` | Switch light 4 ON |
| `LIGHT4OFF` | Switch light 4 OFF |

Switching a light to the state it is already in is accepted and is a no-op (FR-003 acceptance
scenario 3) — the light's output does not change and no report is sent (see below).

### Invalid input handling

- A line naming a light number outside `1`–`4` (e.g. `LIGHT5ON`, `LIGHT0OFF`) → **rejected**
  (`ParseResult::OutOfRange`), discarded, no light's state changes (FR-007).
- A line that isn't a recognized light word at all — including every existing drive word/numeric
  pair — is `ParseResult::Malformed` to this parser and is handed to the existing
  `DriveCommandAssembler` unchanged (FR-008, research.md §3). A line that is malformed to *both*
  parsers (e.g. `LIGHT1MAYBE`, garbage) is discarded with no state change to either lights or
  drive axes, consistent with existing malformed-command handling.

## Outgoing: light state report

Sent by the ESP32 to the app via `BluetoothSerial::println()`, one line terminated `\r\n`:

```
LIGHT_ON | LIGHT_OFF
```

`LIGHT_ON` iff all four lights are currently ON; `LIGHT_OFF` otherwise (i.e. one or more of the
four is OFF). The app is only ever told this aggregate — it has no way to distinguish "1 of 4 on"
from "3 of 4 on" from the wire; both report `LIGHT_OFF`. Per-light identity is a hardware/physical
detail the app doesn't need (updated decision — supersedes the original per-light `LIGHTS:bbbb`
bitmask design).

| When | Report sent |
|------|-------------|
| A light command (`LIGHT<n>ON`/`OFF`) actually changes that light's physical state | `LIGHT_ON` or `LIGHT_OFF`, whichever the resulting aggregate state is (FR-004) |
| A light command names a light already in the requested state (no-op) | Not sent |
| A new Bluetooth connection is established (including reconnection) | `LIGHT_ON` or `LIGHT_OFF` reflecting the current aggregate state (FR-005) |

Note the report can repeat the same value across consecutive sends: e.g. turning on light 2 while
light 1 is already on sends `LIGHT_OFF` (2 of 4 on) same as it would have before that command (1
of 4 on) — a real physical change still triggers a send even when the aggregate string doesn't
change, since the trigger is "a light's state changed," not "the report text changed."

## Versioning

Non-breaking extension of protocol v1 (see `specs/001-bluetooth-motor-control`'s contract) —
adds eight new recognized words and one new ESP32→app report line; no existing word, numeric
form, or report changes shape or meaning.
