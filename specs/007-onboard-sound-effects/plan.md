# Implementation Plan: Onboard Sound Effects

**Branch**: `007-onboard-sound-effects` | **Date**: 2026-09-15 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/007-onboard-sound-effects/spec.md`

**Note**: This plan supersedes an earlier "A2DP Audio Streaming" plan for the same feature slot,
corrected mid-design (see spec.md Clarifications) from Bluetooth audio streaming to fixed,
onboard sound effects triggered over the existing control connection. Any prior `A2dpAudioSink`/
`IAudioConnection`/`ESP32-A2DP` references belong to that superseded design and are being removed
as part of this plan's implementation.

## Summary

Adds one one-shot trigger command (`HORN`) to the existing Bluetooth wire protocol, following the
same "Protocol parses → Control decides → Hardware acts" shape `006`'s light commands already
established. A new `parseHornCommand()` (Protocol) parses `HORN`; a new `ToneControl` (Control)
decides whether to honor a horn trigger based on whether the horn output reports itself busy
(ignoring the request otherwise — spec FR-005); a new `IToneOutput`/`PwmToneOutput` (Hardware)
drives a single GPIO (`config::kToneOutputPin` = 26) via the ESP32's LEDC tone-generation
peripheral: a constant 420Hz tone for 1500ms after `playHorn()`, silent otherwise. Playback is
updated non-blockingly from `tick()` so drive commands are never delayed and the horn is never cut
short (FR-007/FR-008). No Bluetooth audio profile, pairing, or third-party library is used — the
audio is entirely local to the ESP32, one GPIO, one speaker/buzzer. (Two further effects were
implemented then removed: siren — the car is not an ambulance — and an automatic idle/running
engine tone with horn-over-engine layering, dropped after on-device testing. `SIREN` and `ENGINE`
are no longer recognized words.)

## Technical Context

**Language/Version**: C++ (Arduino core for ESP32, C++17 where the toolchain allows), via
PlatformIO — same as `001`/`002`/`004`/`006`.

**Primary Dependencies**: None new. The ESP32 Arduino core's built-in LEDC tone-generation API
(the same peripheral family `ESP32Servo`/`PwmSteeringServo` already use), confirmed by an actual
`pio run -e esp32dev` build to be the channel-based form (`ledcSetup`/`ledcAttachPin`/
`ledcWriteTone`) on the installed platform version — research.md §5, §8. (Supersedes the earlier
draft's `pschatzmann/ESP32-A2DP` dependency, which is being removed.)

**Storage**: N/A — no persisted state; playback-in-progress resets to "not busy" on every boot,
same as every other feature's boot defaults.

**Testing**: Unity via `pio test -e native`. New host-testable logic: `parseHornCommand()`
(Protocol) and `ToneControl` (Control) — both pure, no Arduino dependency — plus one integration
test: `test_tone_command_to_output_flow.cpp` (received `HORN` line → honored-or-ignored →
`playHorn()`, against `FakeTransport`/`FakeToneOutput`).
`PwmToneOutput` itself is a thin ESP32-only wrapper around the LEDC peripheral (like
`PwmSteeringServo`/`GpioMotorDriver`/`GpioLightsOutput`) and is validated on-device via
`quickstart.md`, not a host unit test — same established precedent.

**Target Platform**: ESP32 Dev Module (`esp32dev`, Arduino framework) for firmware; `native` host
environment for tests (Constitution Principle II).

**Project Type**: Single embedded firmware project (existing PlatformIO layout: `src/`, `test/`).

**Performance Goals**: Per spec SC-001 (no perceptible delay triggering a tone), SC-002 (no added
delay/missed drive commands while a tone plays).

**Constraints**: FR-007/FR-008's "never delay drive commands, never cut off a playing horn" is
satisfied by keeping `PwmToneOutput`'s `playHorn()`/`tick()` O(1) per call — at most one LEDC
tone write, never busy-waiting (research.md §7) — not by running on a separate task (there is no
separate task; everything is on the single Arduino main loop, unlike the superseded A2DP design).
No app-side volume control (spec FR-010 — hardware trimpot only). Horn duration is fixed at 1500ms
(`config::kHornDurationMs`) per explicit instruction.

**Scale/Scope**: Single ESP32, one manually-triggered sound effect (horn), one existing Bluetooth
connection (no new connection type).

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Check | Result |
|-----------|-------|--------|
| I. Simplicity First (YAGNI) | No new dependency (research.md §8 — the earlier A2DP library is removed). Small pieces, each mirroring an existing pattern: `parseHornCommand`/`ToneControl`/`IToneOutput`+`PwmToneOutput` mirror `parseLightCommand`/`LightsControl`/`ILightsOutput`+`GpioLightsOutput`. The engine tone and horn-over-engine layering were removed as unneeded (research.md §9). No new abstraction beyond what FR-001–FR-010 require. | PASS |
| II. Test-First | `parseHornCommand()` and `ToneControl` are pure logic and get full unit tests before being wired into `main.cpp`; the horn trigger path gets an integration test against fakes (`FakeTransport`, `FakeToneOutput`). `PwmToneOutput` is validated on-device per existing precedent (`001`/`002`/`004`/`006`), not a gap — see Testing above. | PASS |
| III. Simple, Layered Architecture | `Protocol` (`parseHornCommand`) → `Control` (`ToneControl` decides) → `Hardware` (`PwmToneOutput` acts), one-way, exactly like the existing `LightCommand`/`LightsControl`/`GpioLightsOutput` flow. `main.cpp` remains the only place concrete implementations are wired together and the only place a decided value (horn honored) is handed from `Control` to `Hardware`. No layer gains a new *upward* dependency. | PASS |
| IV. Hardware Abstraction for Testability | `PwmToneOutput` sits behind a new `IToneOutput` interface with `FakeToneOutput` as its test double — same pattern as `IMotorDriver`/`FakeMotorDriver`, `ILightsOutput`/`RecordingLightsOutput`. No hardware access happens outside this interface or the existing ones. | PASS |
| V. Safe Motor Control | Not applicable — this feature adds no motor/servo control and does not read or modify anything `DirectionControl`/`MotorServoVehicleOutput` depend on. The horn is explicitly excluded from the fail-safe scope, mirroring `006`'s lights precedent (spec FR-009, Assumptions): a disconnect/timeout continues to stop the DC motor and center the servo exactly as before. | PASS (N/A to motor control; scope deliberately excluded per spec) |

No unjustified violations. Complexity Tracking table is not needed.

*Re-checked after Phase 1 design (data-model.md, contracts/, quickstart.md): the design introduces
exactly the pieces anticipated above — one Protocol addition, one Control class, one Hardware
interface + impl, and nothing else (no dependency, no new connection
type). All five principles still PASS with no new violations.*

## Project Structure

### Documentation (this feature)

```text
specs/007-onboard-sound-effects/
├── plan.md              # This file (/speckit-plan command output)
├── research.md          # Phase 0 output (/speckit-plan command)
├── data-model.md         # Phase 1 output (/speckit-plan command)
├── quickstart.md         # Phase 1 output (/speckit-plan command)
├── contracts/            # Phase 1 output (/speckit-plan command)
└── tasks.md              # Phase 2 output (/speckit-tasks command - NOT created by /speckit-plan)
```

### Source Code (repository root)

```text
src/
├── config.h                          # kToneOutputPin (GPIO26), kToneLedcChannel, horn duration/
│                                      #   frequency constants (re-purposed from the superseded
│                                      #   A2DP draft)
├── control/
│   └── tone_control.h/.cpp           # NEW: ToneControl::apply(hornBusy)
├── protocol/
│   └── tone_command_parser.h/.cpp    # NEW: parseHornCommand()
├── hardware/
│   ├── i_tone_output.h               # NEW: IToneOutput::playHorn()/hornBusy()/tick()
│   └── pwm_tone_output.h             # NEW: ESP32-only real impl — LEDC tone generation
└── main.cpp                          # + ToneControl/PwmToneOutput instances; loop() tries
                                       #   parseHornCommand() alongside the existing
                                       #   parseLightCommand() check; on Ok, calls
                                       #   toneControl.apply(toneOutput.hornBusy()) and
                                       #   toneOutput.playHorn() if honored;
                                       #   toneOutput.tick() called every loop() iteration

test/test_native/
├── fakes/
│   └── fake_tone_output.h                  # NEW: IToneOutput test double
├── test_tone_command_parser.cpp            # NEW
├── test_tone_control.cpp                   # NEW
└── test_tone_command_to_output_flow.cpp    # NEW: integration, FakeTransport + FakeToneOutput
                                             #   (horn trigger path)
```

Removed: the engine tone (`setEngineRunning()`, `motorEngaged()`, the idle/running/layering
constants, `test_drive_to_engine_tone_flow.cpp`). Also removed (belonged only to the superseded A2DP design, never reached `main.cpp`):
`src/hardware/i_audio_connection.h`, `test/test_native/fakes/fake_audio_connection.h`,
`test/test_native/test_audio_connection_logging_flow.cpp`, the `pschatzmann/ESP32-A2DP`
`lib_deps` entry, and their `test_main.cpp` registrations.

**Structure Decision**: Single embedded project, unchanged layout. Every new piece slots into the
existing `protocol/`, `control/`, `hardware/` directories following the exact same
interface-plus-real-implementation pattern already established (`ILightsOutput`/
`GpioLightsOutput`). `main.cpp` remains the only place concrete implementations are wired
together, same as every prior feature.

## Complexity Tracking

*No violations — table intentionally left empty.*
