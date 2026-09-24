# ambullrc-esp32 — ESP32 Car Engine Controller

Firmware for an **ESP32** that receives commands over **Bluetooth** from an **Android app**
and drives two motors on a hobby car:

- **1 servo motor** — steering / directional control
- **1 DC motor** — engine (drive) control

This is a hobby side project. The guiding rule is **simplicity with quality**: build only what
is needed (YAGNI), keep the architecture small, and cover every feature with automated tests. The Android App which is the remote controller for this device and its componentes are [here](https://github.com/felipexw/ambullRC-ESP32).

[demo (WIP)](https://www.youtube.com/watch?v=EBZQrzWVDxw&list=PLal27zLmiqjQ&index=1)

## Status

Toolchain is set up (PlatformIO, `esp32dev` board). The Transport/Protocol/Control/Hardware
layers described below are implemented: `src/main.cpp` wires up the Bluetooth (BLE GATT)
transport, command parsing, steering-servo and DC-motor control, auxiliary lights, and an
onboard horn tone, with fail-safe behavior on disconnect/timeout.

## How it works

```
Android app  --Bluetooth-->  ESP32
                              ├─ Transport   (Bluetooth send/receive)
                              ├─ Protocol    (parse/validate commands — pure logic)
                              ├─ Control     (servo + DC-motor decisions — pure logic)
                              └─ Hardware    (thin drivers behind interfaces)
                                              ├─ Servo
                                              └─ DC motor
```

Dependencies flow one way (Transport → Protocol → Control → Hardware). Control and protocol
logic never call hardware directly — they go through interfaces, which is what lets the logic
be tested on a normal computer without an ESP32 attached.

## Safety

Because the DC motor drives a physical vehicle, the firmware **fails safe**: on Bluetooth
disconnect, a malformed command, or a command timeout, the DC motor stops and the servo returns
to a defined neutral. Motor outputs are clamped to configured safe ranges. This behavior is
covered by automated tests.

## Testing

Every feature is covered by automated tests before it is considered done:

- **Unit tests** — command parsing, control logic, and state transitions in isolation.
- **Integration tests** — the full command path (Bluetooth command in → motor action out)
  against a fake hardware layer.

The test suite runs on a host machine, no physical ESP32 required.

## Toolchain

Firmware is built with **[PlatformIO](https://platformio.org)**, driven by the `pio` CLI and
configured in [`platformio.ini`](platformio.ini). It handles compiling, flashing over USB, and
running the host test suite, so you don't need the Arduino IDE or a hand-rolled ESP-IDF setup.

- **Board:** `esp32dev` (generic ESP32 DevKit board, CP2102 USB-UART bridge)
- **Framework:** Arduino for ESP32, built on **ESP-IDF v5.4.2**
- **Platform:** `espressif32` @ 7.0.1

```
pio run -e esp32dev              # compile firmware
pio run -e esp32dev -t upload    # compile + flash to a connected ESP32 over USB
pio device monitor -b 115200     # view serial output from the board
pio test -e native               # run the full test suite on the host, no ESP32 needed
pio device list                  # list connected serial ports (find yours if upload can't autodetect)
```

### macOS USB driver

The CP2102 chip needs the Silicon Labs CP210x VCP driver, which macOS doesn't include by
default:

```
brew install --cask silicon-labs-vcp-driver
```

After installing, macOS blocks it until you approve it manually: **System Settings → General →
Login Items & Extensions → Driver Extensions**, enable `com.silabs.cp210x`, then **restart your
Mac** — a device replug alone is not enough for the driver process to actually start. Once
approved and restarted, the board shows up as `/dev/cu.usbserial-*` or `/dev/cu.SLAB_USBtoUART`.

## Repository layout

```
.specify/memory/constitution.md   # Project principles (source of truth)
AGENTS.md                         # Guidance for AI coding agents / contributors
CLAUDE.md                         # Pointer to AGENTS.md for Claude Code
README.md                         # This file
platformio.ini                    # PlatformIO project config (board, framework, envs)
src/                               # Firmware source
test/                              # Tests (native/host-runnable)
```

## Power distribution

| From | To | Notes |
|---|---|---|
| Battery (+) | ON/OFF switch | External switch, non-polarized (2-terminal) |
| Switch out | LM2596 #1 IN+ and LM2596 #2 IN+ | Both regulators fed in parallel from the battery |
| Battery (−) | Common GND | Direct, no switch |
| LM2596 #1 OUT+ | **VCC-A bus (6V)** | Powers L9110S #1 + traction motor |
| LM2596 #2 OUT+ | **VCC-B bus (5V)** | Powers ESP32, LM386, power LED |

## Drivetrain — traction

| From | To | Notes |
|---|---|---|
| VCC-A | L9110S #1 VCC | Power |
| GPIO18 | L9110S #1 B-IA | Direction/PWM |
| GPIO19 | L9110S #1 B-IB | Direction/PWM |
| L9110S #1 MOTOR B+/B− | Traction motor terminals | No polarity — swap to reverse direction mapping |
| C1 (1000µF, 16–25V) | L9110S #1 VCC/GND | Local smoothing capacitor, placed close to the module |

## Drivetrain — steering

| From | To | Notes |
|---|---|---|
| VCC-A | Micro Servo 90g | Powers the servo motor |
| GND | Micro Servo 90g | Common ground |
| GPIO13 | Micro Servo 90g | PWM steering signal (`kServoPin`) — a positional 180° servo driven directly, no motor driver IC involved |

## Control (ESP32)

| From | To | Notes |
|---|---|---|
| VCC-B | ESP32 VIN | 5V |
| Common GND | ESP32 GND | |
| C2 (100–220µF) | VCC-B / GND | Near ESP32/servo area |

## Lighting

| GPIO | Component | Wiring | Notes |
|---|---|---|---|
| GPIO12 | BLE connection status LED (green) | GPIO12 → 220–330Ω → LED anode → cathode → GND | `kLedPin`; on when connected to the RC app |
| GPIO21 | Auxiliary light 1 | GPIO21 → 220–330Ω → LED anode → cathode → GND | `kLight1Pin`; digital on/off |
| GPIO22 | Auxiliary light 2 | GPIO22 → 220–330Ω → LED anode → cathode → GND | `kLight2Pin`; digital on/off |
| GPIO23 | Auxiliary light 3 | GPIO23 → 220–330Ω → LED anode → cathode → GND | `kLight3Pin`; digital on/off |
| GPIO25 | Auxiliary light 4 | GPIO25 → 220–330Ω → LED anode → cathode → GND | `kLight4Pin`; digital on/off. Also the ESP32's DAC channel 1 — kept free of DAC use since the horn uses DAC channel 2 (GPIO26) |
| — (passive) | Power indicator LED (red) | VCC-B → 220–330Ω → LED anode → cathode → GND | Always on when switch is on, no GPIO involved |

All four auxiliary lights are simple independent ON/OFF toggles — there's no taillight,
headlight, turn-signal, or ambient-light logic in the firmware today.

## Audio (LM386 + speaker)

| From | To | Notes |
|---|---|---|
| VCC-B | LM386 VCC | 5V |
| Common GND | LM386 GND (either of the 2 GND pins) | Both pins are the same net |
| GPIO26 (DAC) | Coupling capacitor (10uF) | AC-couples the DAC output into the LM386 input pin, blocking DC bias |
| 100µF stability cap | LM386 VCC / GND | Local, close to the module — separate from the module's built-in output coupling cap |
| LM386 OUT terminal | Speaker wire 1 | |
| LM386 GND terminal | Speaker wire 2 | Speaker has no polarity |

## Bluetooth

- **BLE GATT server** — existing control channel (motor, steering, LEDs)
- **A2DP sink (planned)** — receives Spotify audio via standard OS-level Bluetooth routing on the phone. Requires the original ESP32 (Classic BT support) — confirmed on this build
- **Known tradeoff:** BLE and A2DP share the same radio; expect occasional audio stutter during control bursts, and control latency spikes during audio playback

## Full GPIO map

All pins are defined in [`src/config.h`](src/config.h).

| GPIO | Function | Notes |
|---|---|---|
| 12 | BLE connection status LED (green) | `kLedPin` |
| 13 | Steering servo (SG90) | `kServoPin`; PWM control signal |
| 18 | Traction L9110S — motor input A | `kMotorPinA` |
| 19 | Traction L9110S — motor input B | `kMotorPinB` |
| 21 | Auxiliary light 1 | `kLight1Pin` |
| 22 | Auxiliary light 2 | `kLight2Pin` |
| 23 | Auxiliary light 3 | `kLight3Pin` |
| 25 | Auxiliary light 4 | `kLight4Pin`; also DAC channel 1 — kept free of DAC use |
| 26 | Audio DAC → LM386 (horn tone) | `kToneOutputPin`; DAC channel 2 |

The red power-on LED is wired directly across VCC-B (through its current-limiting resistor) and
isn't driven by a GPIO.

## Project principles

The non-negotiable rules for this project live in
[`.specify/memory/constitution.md`](.specify/memory/constitution.md):

1. Simplicity First (YAGNI)
2. Test-First (NON-NEGOTIABLE)
3. Simple, Layered Architecture
4. Hardware Abstraction for Testability
5. Safe Motor Control

## License

TBD.
