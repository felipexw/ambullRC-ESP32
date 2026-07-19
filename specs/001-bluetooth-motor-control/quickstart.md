# Quickstart: Bluetooth Motor Control (direction-print slice)

Validates that a Bluetooth-connected client can send drive commands and see the correct
`Direction` printed over serial — with no motor/servo movement in this version.

## Prerequisites

- PlatformIO CLI installed, ESP32 board connected (see `AGENTS.md` for the CP210x driver note).
- A way to send raw text lines over Bluetooth SPP to the board, e.g. the "Serial Bluetooth
  Terminal" Android app (or any generic SPP terminal app).

## Host-only validation (no ESP32 required)

```
pio test -e native
```

Expected: all unit tests for `command_parser`, `direction_control`, and the
`test_command_to_direction_flow` integration test pass, per `contracts/bluetooth-command-protocol.md`
and `data-model.md`.

## On-device validation

1. Flash the firmware:

   ```
   pio run -e esp32dev -t upload
   ```

2. Open the serial monitor:

   ```
   pio device monitor -b 115200
   ```

3. From your phone, pair with the ESP32 over Bluetooth and open an SPP terminal app; connect to
   the paired device. Confirm `BLUETOOTH_CONNECTED` is printed once the connection is established.

4. Send each line below (per `contracts/bluetooth-command-protocol.md`) and confirm the matching
   `Direction` appears in the serial monitor:

   | Send | Expect printed |
   |------|-----------------|
   | `0,0` | `STOP` |
   | `0,100` | `FORWARD` |
   | `0,-100` | `BACKWARD` |
   | `-50,0` | `LEFT` |
   | `50,0` | `RIGHT` |
   | `-50,100` | `FORWARD_LEFT` |
   | `abc` (garbage) | *(nothing printed; no crash, monitor stays alive)* |
   | `200,0` (out of range) | *(nothing printed; last direction unchanged)* |

5. Disconnect the terminal app (or stop sending). Confirm `BLUETOOTH_DISCONNECTED` is printed
   once, immediately, and `STOP` is printed once shortly after (on timeout), with no further
   prints until a new valid command is sent.

6. Reconnect. Confirm `BLUETOOTH_CONNECTED` is printed again.

## Out of scope for this validation

No servo or DC-motor movement occurs at any point — this slice only proves the
receive → parse → decide → print path described in `plan.md`.
