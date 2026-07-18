# ambullrc-esp32 — ESP32 Car Engine Controller

Firmware for an **ESP32** that receives commands over **Bluetooth** from an **Android app**
and drives two motors on a hobby car:

- **1 servo motor** — steering / directional control
- **1 DC motor** — engine (drive) control

This is a hobby side project. The guiding rule is **simplicity with quality**: build only what
is needed (YAGNI), keep the architecture small, and cover every feature with automated tests.

## Status

Early setup. Project principles are defined; source code and toolchain are not yet in place.

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

> Build and test commands will be documented here once the toolchain is chosen
> (see [AGENTS.md](AGENTS.md)).

## Repository layout

```
.specify/memory/constitution.md   # Project principles (source of truth)
AGENTS.md                         # Guidance for AI coding agents / contributors
CLAUDE.md                         # Pointer to AGENTS.md for Claude Code
README.md                         # This file
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
