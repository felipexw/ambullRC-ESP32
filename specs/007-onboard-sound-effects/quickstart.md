# Quickstart: Onboard Sound Effects

Validates that `HORN` over the existing Bluetooth control connection plays the horn tone (and
nothing else ever makes a sound), that a horn retrigger while playing is ignored, and that drive
commands and horn playback never interfere with each other — per
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

Existing tests are unaffected — no existing behavior changes shape.

## On-device validation

1. Flash and open the serial monitor:

   ```
   pio run -e esp32dev -t upload
   pio device monitor -b 115200
   ```

2. Pair the phone with `"ambullrc-esp32"` as usual (the existing single connection — no separate
   audio pairing step).

3. Immediately after boot, before sending anything, confirm the speaker is silent.

4. Send `UP`, `DOWN`, `LEFT`, `RIGHT`, and `STOP`. Confirm the speaker stays silent for all of
   them — drive commands never make a sound.

5. Send `HORN`. Confirm the horn tone is audible from the car's speaker (FR-002) and plays for
   exactly ~1500ms (`config::kHornDurationMs`) before going silent again (FR-004, FR-006).

6. While the horn tone is still playing, send `HORN` again. Confirm nothing changes audibly — the
   original horn continues uninterrupted, with no restart or overlap (FR-005, SC-003).

7. While the horn is playing, send drive commands (steering/throttle) at a normal driving
   cadence. Confirm: (a) the car responds with no added delay versus driving with no horn playing
   (SC-002, FR-007), and (b) the horn is not cut short by the drive commands — it plays to its
   natural end regardless (FR-008).

8. Send `ENGINE`. Confirm nothing happens (the word was removed; it's treated as malformed).

9. Drop the Bluetooth connection (move out of range or power-cycle) while driving. Confirm the
   existing DC-motor-stop/servo-neutral fail-safe behavior proceeds exactly as it does without
   this feature (SC-004, FR-009).

## Out of scope for this validation

Any outgoing report/acknowledgement to the app for a horn trigger (ignored or honored) — the app
is not told which happened, per `contracts/tone-command-protocol.md`. Volume/tone quality tuning
and the physical speaker/amp/trimpot circuit itself — see spec Assumptions.
