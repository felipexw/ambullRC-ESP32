# Quickstart: Connection Status LED

Validates that the external connection-status LED tracks the Bluetooth link between the app and
the ESP32, per `contracts/connection-status-led-signal-contract.md`.

## Prerequisites

- PlatformIO CLI installed, ESP32 board wired with an LED (with an appropriate current-limiting
  resistor — the circuit itself is out of scope of this feature) between `config::kLedPin` and
  ground.
- A way to connect/disconnect a Bluetooth SPP client, e.g. the "Serial Bluetooth Terminal" Android
  app (same as `001-bluetooth-motor-control`'s quickstart).
- A multimeter or the LED's visible light is sufficient to confirm ON (3.3V) vs. OFF (0V) — no
  special equipment required.

## Host-only validation (no ESP32 required)

```
pio test -e native
```

Expected: all existing tests still pass unchanged — this feature adds no new host-testable logic.
The event contract `LedConnectionOutput` relies on is already covered by
`test_connection_monitor.cpp` and `test_connection_logging_flow.cpp` (`ConnectionMonitor` →
`IConnectionOutput`, exactly once per transition, tagged with the peer's device ID).

## On-device validation

1. Flash the firmware and open the serial monitor, as in `001-bluetooth-motor-control`'s
   quickstart:

   ```
   pio run -e esp32dev -t upload
   pio device monitor -b 115200
   ```

2. Before pairing, confirm the LED is OFF (FR-003) — it must default OFF at boot, before any
   connection has ever been made.

3. Pair the phone and connect an SPP terminal app. Confirm both:
   - `BLUETOOTH_CONNECTED ...` prints in the serial log (unchanged, existing behavior), and
   - the LED switches ON at the same time, with no perceptible delay (SC-001).

4. Disconnect the terminal app. Confirm both:
   - `BLUETOOTH_DISCONNECTED ...` prints (unchanged, existing behavior), and
   - the LED switches OFF at the same time, with no perceptible delay (SC-002).

5. Repeat connect → disconnect at least 5 times in a row. Confirm the LED matches the actual
   connection state after every single cycle, with no stuck or stale state (SC-003, FR-005).

6. While connected (LED ON), send drive commands and then stop sending them for longer than
   `config::kCommandTimeoutMs` **without disconnecting the Bluetooth link**. Confirm the motor/servo
   enter their existing safe state (unchanged behavior from `001`/`002`/`003`), but the LED
   **stays ON** — the LED only reacts to the Bluetooth link itself, not to command staleness (see
   `research.md` §1 and `contracts/connection-status-led-signal-contract.md`).

7. Power-cycle the board while connected. Confirm the LED comes back up OFF on boot (state does
   not persist across resets), then switches ON again once the app reconnects.

## Out of scope for this validation

The physical LED circuit (resistor sizing, mounting outside the chassis) and any "connecting/
pairing in progress" intermediate visual state are not part of this feature — see spec
Assumptions.
