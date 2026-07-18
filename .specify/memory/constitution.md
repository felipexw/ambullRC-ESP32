<!--
Sync Impact Report
==================
Version change: (unversioned template) → 1.0.0
Rationale: Initial ratification of the project constitution (MAJOR: first adoption
           establishing the full principle set and governance model).

Modified principles: N/A (initial adoption)
Added sections:
  - Core Principles: I. Simplicity First (YAGNI); II. Test-First (NON-NEGOTIABLE);
    III. Simple, Layered Architecture; IV. Hardware Abstraction for Testability;
    V. Safe Motor Control
  - Technical Constraints & Standards
  - Development Workflow & Quality Gates
  - Governance
Removed sections: None

Templates requiring updates:
  - .specify/templates/plan-template.md ✅ reviewed (generic Constitution Check gate, no edits needed)
  - .specify/templates/spec-template.md ✅ reviewed (generic, no edits needed)
  - .specify/templates/tasks-template.md ✅ reviewed (test tasks supported; testing is
    mandatory per Principle II — /speckit-tasks MUST include test tasks for this project)
  - README / quickstart docs: none present (nothing to update)

Follow-up TODOs: None
-->

# ESP32 Car Engine Controller Constitution

## Core Principles

### I. Simplicity First (YAGNI)

Build only what a working hobby project needs today. Every feature, abstraction, library,
configuration option, or layer MUST be justified by a concrete, current requirement — never
by a hypothetical future one. When two designs satisfy the same requirement, the simpler one
MUST win. Speculative generality, unused parameters, plugin systems, and premature
optimization are prohibited.

Rationale: This is a single-purpose side project. Complexity has an ongoing maintenance cost
that a hobby project cannot afford; keeping the surface area small is what preserves quality
over time.

### II. Test-First (NON-NEGOTIABLE)

Every new feature MUST be covered by automated tests before it is considered complete. Both
levels are required where applicable:

- **Unit tests** MUST cover command parsing, control logic, and state transitions in isolation
  from hardware.
- **Integration tests** MUST cover the end-to-end command path (Bluetooth command in →
  motor-control action out) against a hardware abstraction, not physical motors.

No feature is merged without passing tests. Bug fixes MUST add a regression test that fails
before the fix and passes after. Tests MUST be runnable on a host machine (native/emulated),
without requiring a physical ESP32 attached, so the suite runs in CI and locally.

Rationale: The device drives a physical car engine; automated tests are the cheapest reliable
guard against regressions that would otherwise only surface as unpredictable hardware behavior.

### III. Simple, Layered Architecture

The system MUST be organized into a minimal set of clearly separated layers with one-way
dependencies:

1. **Transport** — Bluetooth send/receive only.
2. **Protocol** — parse/validate/serialize commands; pure logic, no I/O.
3. **Control** — servo and DC-motor decision logic; pure logic, no direct hardware calls.
4. **Hardware** — thin drivers behind an interface (servo, DC motor, GPIO).

Business/control logic MUST NOT call hardware APIs directly; it goes through the hardware
interface. Layers MUST NOT reach "upward." No additional layers, frameworks, or indirection
may be introduced without an explicit justification recorded in the plan's Complexity Tracking.

Rationale: This is the minimum structure that keeps logic testable off-device (Principle II
and IV) while staying small enough to hold in one's head.

### IV. Hardware Abstraction for Testability

All hardware access (Bluetooth, servo, DC motor, GPIO, timers) MUST sit behind narrow
interfaces with a real implementation for the ESP32 and a fake/mock implementation for tests.
Control and protocol code MUST depend only on these interfaces. Direct hardware register or
vendor-SDK calls outside the hardware layer are prohibited.

Rationale: Abstraction here is not speculative generality — it is the concrete mechanism that
makes Principle II's host-runnable tests possible. It is the one abstraction the project pays
for on purpose.

### V. Safe Motor Control

Motor commands act on a physical vehicle, so safety behavior is mandatory and MUST be tested:

- On Bluetooth disconnect, malformed command, or command timeout, the DC motor MUST default to
  a defined safe state (stopped) and the servo MUST hold or return to a defined neutral.
- Motor outputs MUST be clamped to configured safe ranges; out-of-range commands are rejected,
  not clamped silently past the limit.
- The safe-state behavior MUST be exercised by automated tests.

Rationale: A dropped connection or garbage input must never leave the engine in an
uncontrolled or accelerating state; failing safe is a hard requirement, not a nicety.

## Technical Constraints & Standards

- **Target hardware**: ESP32 microcontroller controlling one servo motor and one DC motor.
- **Interface**: Bluetooth commands originating from an Android application. The command
  protocol MUST be simple, documented, and versioned in the repository.
- **Testing environment**: The test suite MUST run on a host machine without a physical ESP32.
  Hardware-in-the-loop testing MAY supplement automated tests but MUST NOT replace them.
- **Dependencies**: Prefer the standard ESP32 toolchain and libraries already in use. Adding a
  new third-party dependency requires justification against Principle I in the plan.
- **No hidden state**: Configuration (pin assignments, safe ranges, timeouts) MUST be explicit
  and centralized, not scattered as magic numbers.

## Development Workflow & Quality Gates

- Every change MUST pass the automated test suite before merge; a merge with failing or absent
  required tests is prohibited (Principle II).
- Every plan produced by `/speckit-plan` MUST pass the Constitution Check gate. Any principle
  violation MUST be recorded in the plan's Complexity Tracking table with justification and the
  rejected simpler alternative, or the plan MUST be revised.
- Task lists produced by `/speckit-tasks` MUST include unit and integration test tasks for each
  user story; testing is mandatory for this project and is not optional.
- Code review MUST verify: tests exist and pass, layering (Principle III) is respected,
  hardware access stays behind interfaces (Principle IV), and safe-state behavior (Principle V)
  is covered.

## Governance

This constitution supersedes other development practices for this project. When a practice and
this document conflict, this document wins.

- **Amendments**: Proposed by editing this file with a clear rationale. An amendment takes
  effect once merged. Dependent templates and docs MUST be updated in the same change.
- **Versioning**: This constitution follows semantic versioning.
  - **MAJOR**: Backward-incompatible governance changes or removal/redefinition of a principle.
  - **MINOR**: A new principle or section, or materially expanded guidance.
  - **PATCH**: Clarifications, wording, or typo fixes with no semantic change.
- **Compliance review**: Plans and pull requests MUST be checked against these principles.
  Complexity that violates a principle MUST be justified in the plan's Complexity Tracking or
  removed. Unjustified violations block the change.

**Version**: 1.0.0 | **Ratified**: 2026-07-17 | **Last Amended**: 2026-07-17
