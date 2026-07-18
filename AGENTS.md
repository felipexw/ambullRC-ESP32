# AGENTS.md

Guidance for AI coding agents and human contributors working in this repository.

## What this project is

Firmware for an **ESP32** that receives commands over **Bluetooth** from an **Android app** and
controls two motors on a hobby car: one **servo motor** (steering) and one **DC motor** (engine).

It is a hobby side project. Optimize for **simplicity and quality**, not for scale or features.

## Source of truth

The project constitution is authoritative. Read it before making design decisions:
[`.specify/memory/constitution.md`](.specify/memory/constitution.md).

If anything here conflicts with the constitution, the constitution wins.

## Core rules (from the constitution)

1. **Simplicity First (YAGNI).** Build only what a current requirement needs. Prefer the
   simpler design. No speculative abstractions, unused options, or premature optimization.
2. **Test-First (NON-NEGOTIABLE).** Every feature gets automated tests before it is done.
   Bug fixes add a regression test that fails before the fix. Tests must run on a host machine
   without a physical ESP32.
3. **Simple, Layered Architecture.** One-way dependencies:
   `Transport → Protocol → Control → Hardware`. Logic never calls hardware directly.
4. **Hardware Abstraction for Testability.** All hardware (Bluetooth, servo, DC motor, GPIO,
   timers) sits behind narrow interfaces with a real ESP32 implementation and a fake for tests.
   No vendor-SDK/register calls outside the hardware layer.
5. **Safe Motor Control.** On disconnect, malformed command, or timeout: DC motor stops, servo
   goes neutral. Outputs are clamped to safe ranges. Safe-state behavior must be tested.

## Architecture layers

| Layer | Responsibility | May call | Testable off-device |
|-------|----------------|----------|---------------------|
| Transport | Bluetooth send/receive | Hardware iface | via fake transport |
| Protocol | Parse / validate / serialize commands (pure) | nothing (no I/O) | yes, directly |
| Control | Servo + DC-motor decision logic (pure) | Hardware iface | yes, directly |
| Hardware | Thin drivers: servo, DC motor, GPIO, timers | ESP32 SDK | via fakes |

- Keep Protocol and Control **pure** (no I/O, no hardware calls) so they are trivially unit-tested.
- Put every hardware touch behind an interface so Control can run against a fake in tests.

## Definition of done for any change

- [ ] Unit tests cover new/changed parsing, control logic, and state transitions.
- [ ] Integration tests cover the affected command path (command in → motor action out) against
      a fake hardware layer.
- [ ] Safe-state behavior is preserved and tested if the change touches motor control.
- [ ] Layering respected; no hardware access outside the Hardware layer.
- [ ] The full test suite passes on the host.
- [ ] No new dependency or abstraction added without a concrete, current justification.

## Toolchain

> **Not yet chosen.** When the build system is selected (e.g., PlatformIO with a `native` test
> environment, ESP-IDF, or Arduino), record the exact build/flash/test commands here and in the
> README. A host-runnable test environment is a hard requirement (Principle II), so favor a
> toolchain that supports native/host unit testing.

Once established, this section should contain the concrete commands, for example:

```
# build firmware
<TBD>

# run the full test suite on the host (no ESP32 needed)
<TBD>

# flash to a connected ESP32
<TBD>
```

## Conventions

- Centralize configuration (pin assignments, safe motor ranges, timeouts) — no magic numbers
  scattered through the code.
- Keep the Bluetooth command protocol simple, documented, and versioned in the repo.
- Match the style, naming, and structure of surrounding code.
- Prefer small, focused changes; commit after each logical unit of work.

## Spec Kit workflow

This repo uses Spec Kit. Feature work flows through `/speckit-specify` → `/speckit-plan` →
`/speckit-tasks` → `/speckit-implement`. Plans must pass the Constitution Check gate, and task
lists must include unit and integration test tasks (testing is mandatory here, not optional).
