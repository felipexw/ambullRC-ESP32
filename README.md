# ambullrc-esp32 — ESP32 Car Engine Controller

Firmware for an **ESP32** that receives commands over **Bluetooth** from an **Android app**
and drives two motors on a hobby car:

- **1 servo motor** — steering / directional control
- **1 DC motor** — engine (drive) control

This is a hobby side project. The guiding rule is **simplicity with quality**: build only what
is needed (YAGNI), keep the architecture small, and cover every feature with automated tests.

## Status

Toolchain is set up (PlatformIO, `esp32dev` board). `src/main.cpp` is currently a smoke-test
sketch confirming the build/flash pipeline; the real Transport/Protocol/Control/Hardware layers
described below are not yet implemented.

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
