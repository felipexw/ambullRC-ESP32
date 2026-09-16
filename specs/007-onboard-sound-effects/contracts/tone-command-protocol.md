# Tone Command Protocol Contract

Satisfies FR-001, FR-005, FR-006. This is the wire-format contract for triggering the horn over
the existing Bluetooth control connection — no new connection type, per spec Clarifications. The
engine sound is fully automatic (FR-007) and has no wire-format command of its own — see
`tone-output-hardware-contract.md`.

## Incoming command words

| Word (case-insensitive) | `parseHornCommand()` result |
|---|---|
| `HORN` | `ParseResult::Ok` |
| `ENGINE` | `ParseResult::Malformed` — removed as a manual trigger; the engine is now automatic (falls through to the existing parsers, unchanged, same as `SIREN`) |
| Any existing drive word (`UP`, `LEFT`, ...) | `ParseResult::Malformed` (falls through to `DriveCommandAssembler`, unchanged) |
| Any existing light word (`LIGHT1ON`, ...) | `ParseResult::Malformed` (falls through to `parseLightCommand()`, unchanged) |
| A numeric pair (`"0,0"`) | `ParseResult::Malformed` (falls through to `DriveCommandAssembler`, unchanged) |
| Anything else | `ParseResult::Malformed` |

There is no ON/OFF pair and no numeric argument — `HORN` is a single, one-shot trigger event
(spec FR-001), unlike the light commands' stateful toggle.

## Routing (research.md §2)

`main.cpp` tries `parseHornCommand()` alongside the existing `parseLightCommand()` check, before
falling through to `DriveCommandAssembler::apply()`. Order between the horn and light checks does
not matter — their word sets are disjoint.

## Busy behavior (FR-005, FR-006)

A syntactically valid `HORN` (`ParseResult::Ok`) reaching `main.cpp` does **not** guarantee
playback — it is only *forwarded* to `ToneControl::apply()`, which honors it only if
`IToneOutput::hornBusy()` is currently `false` (see `data-model.md`). A `HORN` received while the
horn is already playing is parsed successfully but has no effect on the car (silently ignored,
per spec FR-005 — this is intentional, not an error condition, so no rejection or error is
reported back to the app).

## Out of scope

- No outgoing report line is added for sound effects — unlike the lights feature (`LIGHTS:`
  report), the app is not told whether a horn trigger was honored or ignored. The app finds out
  only by whether it hears the sound (spec is silent on requiring any acknowledgement).
- No wire command exists for the engine sound at all — it is driven entirely by the existing
  drive-command wire format (`UP`/`DOWN`/`STOP`/etc.), which this contract does not change.
