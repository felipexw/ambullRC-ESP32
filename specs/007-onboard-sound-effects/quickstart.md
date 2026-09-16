# Quickstart: Onboard Sound Effects

Validates that `HORN` over the existing Bluetooth control connection plays the horn tone layered
on top of the always-on engine tone, that a horn retrigger while playing is ignored, that the
engine tone automatically switches between idle and running as drive commands come in, and that
drive commands and sound-effect playback never interfere with each other — per
`contracts/tone-command-protocol.md` and `contracts/tone-output-hardware-contract.md`.

## Prerequisites

- PlatformIO CLI installed, ESP32 board with the existing servo/motor/lights/LED wiring
  (`001`/`002`/`004`/`006`) plus a speaker/buzzer connected to `config::kToneOutputPin` (GPIO26),
  with a volume trim potentiometer in that path (hardware provisioning is out of this feature's
  scope — see spec Assumptions).
- A Bluetooth SPP terminal app that can send lines, e.g. "Serial Bluetooth Terminal" (same tool
  used by `001-bluetooth-motor-control`'s quickstart) — the existing single control connection,
  no separate audio pairing.

## Host-only validation (no ESP32 required)

```
pio test -e native
```

Expected new tests, all passing:

- `test_tone_command_parser.cpp` — `HORN` parsing (case-insensitive); `Malformed` for every
  existing drive word, light word, numeric pair, garbage, and the removed `SIREN`/`ENGINE` words
  (so they correctly fall through to the existing parsers, per research.md §2).
- `test_tone_control.cpp` — `apply(hornBusy)` honors a request when `hornBusy` is `false`;
  ignores it (returns `false`) when `hornBusy` is `true`.
- `test_tone_command_to_output_flow.cpp` — end-to-end via `FakeTransport` + `FakeToneOutput`: a
  queued `HORN` line results in exactly one `playHorn()` call; a second `HORN` line queued while
  `FakeToneOutput`'s horn-busy flag is still set results in no additional `playHorn()` call; once
  no longer busy, the next trigger plays again.
- `test_direction_control.cpp` — new `motorEngaged(Direction)` cases: `true` for
  `Forward`/`Backward`/their diagonals, `false` for `Stop`/`Left`/`Right`.
- `test_drive_to_engine_tone_flow.cpp` — end-to-end via `FakeTransport` + `DriveCommandAssembler`
  + `DirectionControl` + `FakeToneOutput`: `UP`/`DOWN` result in `setEngineRunning(true)`; `RIGHT`/
  `LEFT` alone leave it `false`; `STOP` and a simulated disconnect both return it to `false`.

Existing tests are unaffected — no existing behavior changes shape.

## On-device validation

1. Flash and open the serial monitor:

   ```
   pio run -e esp32dev -t upload
   pio device monitor -b 115200
   ```

2. Pair the phone with `"ambullrc-esp32"` as usual (the existing single connection — no separate
   audio pairing step).

3. Immediately after boot, before sending any drive command, confirm a low, idle engine rumble is
   already audible from the car's speaker (spec Assumptions — the engine plays from power-on, not
   just after a command).

4. Send `UP` (or `DOWN`). Confirm the engine sound switches to a noticeably different, "running"
   tone within the same responsiveness window as the DC motor engaging (SC-005, FR-009).

5. Send `STOP` (or release the throttle so the assembler emits `STOP`). Confirm the engine sound
   returns to the idle rumble (FR-008).

6. Send `LEFT` or `RIGHT` alone (no throttle active). Confirm the engine sound stays on the idle
   rumble — steering alone does not switch it to running (FR-008, since the DC motor itself isn't
   engaged).

7. Send `HORN`. Confirm the horn tone is audible layered on top of whichever engine tone is
   currently playing (FR-002, FR-010) — the engine should still be perceptible under/between the
   horn, not silenced — and that it plays for exactly ~1500ms (`config::kHornDurationMs`) before
   dropping out, leaving the engine tone alone again.

8. While the horn tone is still playing, send `HORN` again. Confirm nothing changes audibly — the
   original horn continues uninterrupted, with no restart or overlap (FR-005, SC-003).

9. While a tone is playing (horn, or either engine state), send drive commands (steering/
   throttle) at a normal driving cadence. Confirm: (a) the car responds with no added delay versus
   driving with no horn playing (SC-002, FR-011), and (b) an in-progress horn is not cut short by
   the drive commands — it plays to its natural end regardless (FR-012). The engine tone changing
   in response to those same drive commands is expected (User Story 2), not a defect.

10. Drop the Bluetooth connection (move out of range or power-cycle) while driving. Confirm the
    existing DC-motor-stop/servo-neutral fail-safe behavior proceeds exactly as it does without
    this feature (SC-004, FR-013), and that the engine tone returns to idle in step with the motor
    stopping.

## Out of scope for this validation

Any outgoing report/acknowledgement to the app for a horn trigger (ignored or honored) — the app
is not told which happened, per `contracts/tone-command-protocol.md`. Volume/tone quality tuning,
the physical speaker/amp/trimpot circuit itself, and true simultaneous horn+engine audio mixing
(the time-sliced layering is an approximation) — see spec Assumptions.
