# Quickstart: Motor & Servo Actuation

Validates that a Bluetooth-connected client's drive commands actually move the DC motor and
steering servo — not just print a direction — per `contracts/direction-to-actuation-contract.md`.

## Prerequisites

- PlatformIO CLI installed, ESP32 board wired per the project's checklist (GPIO18/19 → L9110S
  A-IA/A-IB, GPIO13 → servo signal, common ground).
- A way to send raw text lines over Bluetooth SPP, e.g. the "Serial Bluetooth Terminal" Android
  app (same as `001-bluetooth-motor-control`'s quickstart).
- Chassis on a stand or wheels off the ground for the first run (the motor will actually spin).

## Host-only validation (no ESP32 required)

```
pio test -e native
```

Expected: all existing tests still pass, plus the new tests for this feature —
`test_motor_servo_vehicle_output` (Direction → actuation mapping and the reversal-pause timing)
and `test_command_to_actuation_flow` (fake transport → decided direction → both the serial log and
the motor/servo fakes, including a disconnect scenario).

## On-device validation

1. Flash the firmware and open the serial monitor, as in `001-bluetooth-motor-control`'s
   quickstart:

   ```
   pio run -e esp32dev -t upload
   pio device monitor -b 115200
   ```

2. Pair the phone, connect an SPP terminal app, and confirm `BLUETOOTH_CONNECTED ...` prints as
   before.

3. Send each line below and confirm both the logged direction **and** the physical motor/servo
   response, per the contract table:

   | Send | Logged | Motor | Servo |
   |------|--------|-------|-------|
   | `0,100` | `FORWARD` | spins forward | centered |
   | `0,-100` | `BACKWARD` | spins in reverse (see step 4 for the pause) | centered |
   | `-50,0` | `LEFT` | stopped | turns toward full-left lock, then auto-stops |
   | `50,0` | `RIGHT` | stopped | turns toward full-right lock, then auto-stops |
   | `50,100` | `FORWARD_RIGHT` | spins forward | turns toward full-right lock, then auto-stops |
   | `0,0` | `STOP` | stopped | centered |

   The servo is continuous-rotation, not positional: `LEFT`/`RIGHT` spin it for
   `config::kServoTurnPulseMs` (~300ms) then it self-stops, even if you never send another
   command — confirm it does NOT keep spinning indefinitely after a `LEFT`/`RIGHT` send.

4. Send `0,100` (forward), then immediately send `0,-100` (reverse). Confirm the motor visibly
   stops for roughly 300ms before spinning in reverse — it must not snap directly from forward to
   reverse.

5. While the motor is driving (forward or reverse), disconnect the terminal app. Confirm
   `BLUETOOTH_DISCONNECTED ...` prints, the motor stops **immediately** (no 300ms pause on a
   fail-safe stop), and the servo returns to center.

6. Reconnect and send `50,0` (right) followed immediately by disconnecting. Confirm the servo
   returns to center immediately on disconnect, same as step 5.

## Out of scope for this validation

Proportional steering (angle scaling with the `steer` magnitude) and motor speed control (as
opposed to on/off polarity) are not part of this feature — see spec Assumptions.
