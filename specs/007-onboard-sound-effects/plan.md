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
established, plus a fully automatic engine tone that requires no command at all. A new
`parseHornCommand()` (Protocol) parses `HORN`; a new `ToneControl` (Control) decides whether to
honor a horn trigger based on whether the horn output reports itself busy (ignoring the request
otherwise — spec FR-005); a new `IToneOutput`/`PwmToneOutput` (Hardware) drives a single GPIO
(`config::kToneOutputPin` = 26) via the ESP32's LEDC tone-generation peripheral. The engine tone
plays continuously and automatically — `main.cpp` derives whether the DC motor is engaged from the
already-decided `Direction` (via a new `motorEngaged()` helper) and calls
`IToneOutput::setEngineRunning()` every time a drive command is handled, switching the LEDC output
between a low ~52Hz idle rumble and a higher ~147Hz "running" rumble (both wobbled to approximate
a V8). The horn, when triggered, plays on top of whichever engine tone is active by rapidly
time-slicing the single LEDC channel between the horn and engine frequencies (spec FR-010), rather
than silencing the engine — this and the automatic-engine design are updated non-blockingly from
`tick()` so drive commands are never delayed and the horn is never cut short (FR-011/FR-012). No
Bluetooth audio profile, pairing, or third-party library is used — the audio is entirely local to
the ESP32, one GPIO, one speaker/buzzer. (A third effect, siren, was implemented then removed —
the car is not an ambulance. `ENGINE` was also removed as a manual trigger word once the engine
sound became automatic.)

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
(Protocol), `ToneControl` (Control), `motorEngaged()` (Control) — all pure, no Arduino dependency
— plus two integration tests: `test_tone_command_to_output_flow.cpp` (received `HORN` line →
honored-or-ignored → `playHorn()`, against `FakeTransport`/`FakeToneOutput`) and
`test_drive_to_engine_tone_flow.cpp` (received drive command → `Direction` → `motorEngaged()` →
`setEngineRunning()`, against the same fakes plus `DriveCommandAssembler`/`DirectionControl`).
`PwmToneOutput` itself is a thin ESP32-only wrapper around the LEDC peripheral (like
`PwmSteeringServo`/`GpioMotorDriver`/`GpioLightsOutput`) and is validated on-device via
`quickstart.md`, not a host unit test — same established precedent.

**Target Platform**: ESP32 Dev Module (`esp32dev`, Arduino framework) for firmware; `native` host
environment for tests (Constitution Principle II).

**Project Type**: Single embedded firmware project (existing PlatformIO layout: `src/`, `test/`).

**Performance Goals**: Per spec SC-001 (no perceptible delay triggering a tone), SC-002 (no added
delay/missed drive commands while a tone plays).

**Constraints**: FR-011/FR-012's "never delay drive commands, never cut off a playing horn" is
satisfied by keeping `PwmToneOutput`'s `tick()` O(1) per call — updating the LEDC frequency, never
busy-waiting (research.md §7) — not by running on a separate task (there is no separate task;
everything is on the single Arduino main loop, unlike the superseded A2DP design). No app-side
volume control (spec FR-014 — hardware trimpot only). Only the horn has a busy flag; the engine
never stops (spec Assumptions, research.md §4). Horn duration is fixed at 1500ms
(`config::kHornDurationMs`) per explicit instruction. The horn layers on top of the engine via
single-channel time-slicing, not true simultaneous mixing (spec Assumptions, research.md §10).

**Scale/Scope**: Single ESP32, one manually-triggered sound effect (horn) plus one fully automatic
one (engine, idle/running), one existing Bluetooth connection (no new connection type).

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Check | Result |
|-----------|-------|--------|
| I. Simplicity First (YAGNI) | No new dependency (research.md §8 — the earlier A2DP library is removed). Small pieces, each mirroring an existing pattern: `parseHornCommand`/`ToneControl`/`IToneOutput`+`PwmToneOutput` mirror `parseLightCommand`/`LightsControl`/`ILightsOutput`+`GpioLightsOutput`; `motorEngaged()` is a one-line pure mapping added to the existing `control/direction.h`, not a new class (research.md §9). The engine's automatic idle/running behavior reuses the already-decided `Direction` rather than introducing a second "is the car driving" concept. Horn-over-engine layering reuses the single existing LEDC channel via time-slicing rather than adding a second GPIO/speaker (research.md §10 — the added-hardware alternative was explicitly considered and rejected). No new abstraction beyond what FR-001–FR-014 require. | PASS |
| II. Test-First | `parseHornCommand()`, `ToneControl`, and `motorEngaged()` are pure logic and get full unit tests before being wired into `main.cpp`; both the horn trigger path and the drive-command-to-engine-state path get integration tests against fakes (`FakeTransport`, `FakeToneOutput`, `DriveCommandAssembler`, `DirectionControl`). `PwmToneOutput` is validated on-device per existing precedent (`001`/`002`/`004`/`006`), not a gap — see Testing above. | PASS |
| III. Simple, Layered Architecture | `Protocol` (`parseHornCommand`) → `Control` (`ToneControl` decides; `motorEngaged()` maps `Direction`) → `Hardware` (`PwmToneOutput` acts), one-way, exactly like the existing `LightCommand`/`LightsControl`/`GpioLightsOutput` flow. `main.cpp` remains the only place concrete implementations are wired together and the only place a decided value (horn honored, or the current `Direction`) is handed from `Control` to `Hardware`. No layer gains a new *upward* dependency. | PASS |
| IV. Hardware Abstraction for Testability | `PwmToneOutput` sits behind a new `IToneOutput` interface with `FakeToneOutput` as its test double — same pattern as `IMotorDriver`/`FakeMotorDriver`, `ILightsOutput`/`RecordingLightsOutput`. No hardware access happens outside this interface or the existing ones. | PASS |
| V. Safe Motor Control | Not applicable — this feature adds no motor/servo control and does not read or modify anything `DirectionControl`/`MotorServoVehicleOutput` depend on; it only *reads* the already-decided `Direction` via `motorEngaged()`. Sound effects are explicitly excluded from the fail-safe scope, mirroring `006`'s lights precedent (spec FR-013, Assumptions): a disconnect/timeout continues to stop the DC motor and center the servo exactly as before. The engine tone does switch to idle on that same transition, but only as a side effect of reading `Direction::Stop`, not new fail-safe logic. | PASS (N/A to motor control; scope deliberately excluded per spec) |

No unjustified violations. Complexity Tracking table is not needed.

*Re-checked after Phase 1 design (data-model.md, contracts/, quickstart.md): the design introduces
exactly the pieces anticipated above — one Protocol addition, one Control class plus one Control
helper function, one Hardware interface + impl, and nothing else (no dependency, no new connection
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
├── config.h                          # kToneOutputPin (GPIO26), horn/idle/running frequency and
│                                      #   layering constants (re-purposed from the superseded
│                                      #   A2DP draft)
├── control/
│   ├── direction.h                   # + motorEngaged(Direction) helper (NEW addition to an
│   │                                  #   existing file, not a new file)
│   └── tone_control.h/.cpp           # NEW: ToneControl::apply(hornBusy)
├── protocol/
│   └── tone_command_parser.h/.cpp    # NEW: parseHornCommand()
├── hardware/
│   ├── i_tone_output.h               # NEW: IToneOutput::playHorn()/hornBusy()/
│   │                                  #   setEngineRunning()/tick()
│   └── pwm_tone_output.h             # NEW: ESP32-only real impl — LEDC tone generation
└── main.cpp                          # + ToneControl/PwmToneOutput instances; loop() tries
                                       #   parseHornCommand() alongside the existing
                                       #   parseLightCommand() check; on Ok, calls
                                       #   toneControl.apply(toneOutput.hornBusy()) and
                                       #   toneOutput.playHorn() if honored; every decided drive
                                       #   Direction (including the safe-state STOP) also calls
                                       #   toneOutput.setEngineRunning(motorEngaged(direction));
                                       #   toneOutput.tick() called every loop() iteration

test/test_native/
├── fakes/
│   └── fake_tone_output.h                  # NEW: IToneOutput test double
├── test_tone_command_parser.cpp            # NEW
├── test_tone_control.cpp                   # NEW
├── test_tone_command_to_output_flow.cpp    # NEW: integration, FakeTransport + FakeToneOutput
│                                            #   (horn trigger path)
├── test_drive_to_engine_tone_flow.cpp      # NEW: integration, DriveCommandAssembler +
│                                            #   DirectionControl + FakeToneOutput (automatic
│                                            #   engine-state path)
└── test_direction_control.cpp              # + motorEngaged() unit tests (existing file)
```

Removed (belonged only to the superseded A2DP design, never reached `main.cpp`):
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
