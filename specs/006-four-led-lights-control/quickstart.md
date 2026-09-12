# Quickstart: Four-Light Auxiliary Control

Validates that the four auxiliary lights switch on/off independently from app commands and that
the ESP32 reports their state back to the app, per `contracts/light-command-protocol.md` and
`contracts/lights-state-to-gpio-contract.md`.

## Prerequisites

- PlatformIO CLI installed, ESP32 board wired with four LEDs (each with an appropriate
  current-limiting resistor — the circuit itself is out of scope of this feature) between
  `config::kLight1Pin`..`kLight4Pin` and ground.
- A Bluetooth SPP terminal app that can both send lines and display received lines, e.g. "Serial
  Bluetooth Terminal" (same tool used by `001-bluetooth-motor-control`'s quickstart) — needed here
  because this feature, unlike prior ones, requires observing data the ESP32 sends back.

## Host-only validation (no ESP32 required)

```
pio test -e native
```

Expected new tests, all passing:

- `test_light_command_parser.cpp` — `LIGHT<n>ON`/`OFF` parsing, case-insensitivity, `OutOfRange`
  for light numbers outside 1–4, `Malformed` for anything else (so it correctly falls through to
  the existing drive parser).
- `test_lights_report_formatter.cpp` — `LightsState` → `"LIGHT_ON"` (all four ON) /
  `"LIGHT_OFF"` (anything else) formatting.
- `test_lights_control.cpp` — per-light independent state transitions, all-OFF default, no-op
  same-state commands reported as "unchanged."
- `test_light_command_to_report_flow.cpp` — end-to-end via `FakeTransport` +
  `RecordingLightsOutput`: a queued `LIGHT2ON` line results in a recorded `apply()` call with only
  light 2 ON, and a `LIGHT_OFF` line written back to the fake transport (since not all four are
  on); turning on the remaining three sends `LIGHT_ON`; a connect event alone (no light command)
  results in the current aggregate state being written back.

Existing tests are unaffected — no existing behavior changes shape.

## On-device validation

1. Flash and open the serial monitor:

   ```
   pio run -e esp32dev -t upload
   pio device monitor -b 115200
   ```

2. Before pairing, confirm all four lights are OFF (FR-006).

3. Pair the phone with an SPP terminal app that can send and receive.

4. Send `LIGHT1ON`. Confirm:
   - Light 1 turns on physically, with no perceptible delay (SC-001), and lights 2–4 are
     unaffected.
   - The terminal receives `LIGHT_OFF` (only 1 of 4 lights is on — the app is only ever told
     whether *all four* are on).

5. Send `LIGHT2ON`, `LIGHT3ON`, then `LIGHT4ON` (light 1 is still ON from step 4). Confirm each
   command turns on only its own light, the terminal receives `LIGHT_OFF` after the first two
   (still not all four), and `LIGHT_ON` after the last one (now all four are on). Then send
   `LIGHT1OFF`. Confirm only light 1 turns off and the terminal receives `LIGHT_OFF` again.

6. Send `LIGHT1OFF` again while light 1 is already OFF. Confirm nothing changes physically and no
   new report line is sent (FR-004's "whenever state changes").

7. Send an invalid light command, e.g. `LIGHT9ON`. Confirm no light changes and no report line is
   sent (FR-007).

8. With lights 2, 3, 4 ON and light 1 OFF, disconnect and reconnect the terminal app. Confirm the
   terminal immediately receives `LIGHT_OFF` on reconnect, without sending any command (SC-003,
   FR-005), and that no light's physical state changed across the disconnect (FR-009). Then turn
   light 1 back ON, disconnect, and reconnect again — confirm this time it receives `LIGHT_ON`.

9. While a light is ON, drop the Bluetooth connection for longer than `config::kCommandTimeoutMs`
   (triggering the existing motor/servo fail-safe). Confirm the light stays in its last commanded
   state — lights are not part of the motor/servo fail-safe (FR-009).

## Out of scope for this validation

Dimming, blinking, or any semantic meaning assigned to a specific light number (e.g. "light 1 is
the brake light") — the four lights are generic, identical, independently switched outputs, per
spec Assumptions.
