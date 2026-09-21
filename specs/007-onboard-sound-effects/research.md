# Phase 0 Research: Onboard Sound Effects

This supersedes the research done for the earlier "A2DP Audio Streaming" scoping of this feature
(see spec.md Clarifications for why). No Bluetooth audio profile, pairing, or third-party A2DP
library is needed at all — sound effects are fixed, stored on the ESP32, and triggered over the
existing control connection.

## 1. Wire format for the trigger command

**Decision**: Extend the existing word-command vocabulary (`UP`/`DOWN`/`LEFT`/`RIGHT`/`STOP`/
`CENTER`, `LIGHT<n>ON`/`OFF`) with one new one-shot trigger word: `HORN` — case-insensitive, no
delimiters, one per line, parsed by a new `parseHornCommand()` reusing the existing `ParseResult`
enum (`Ok`/`Malformed`; `OutOfRange` doesn't apply — there's no numeric argument to be out of
range). Two other words were considered and both dropped: `SIREN` (the car is not an ambulance)
and `ENGINE` (the engine sound was tried as an automatic effect and then removed entirely — see
§9).

**Rationale**: Same reasoning as `006`'s light commands (research.md §1 there): the app already
sends single, self-contained words per button press, and a one-shot trigger is naturally the same
shape. Unlike lights, there's no ON/OFF pair — pressing the horn button is a single event, not a
state toggle, so one word is sufficient.

**Alternatives considered**: A single generic word plus an argument (`SOUND HORN` / `SFX1`) —
rejected as an unnecessary indirection; a fixed, self-describing word is simpler and matches the
existing per-button-press wire shape exactly. `SIREN` — removed after initial implementation; not
fitting the vehicle. `ENGINE` — removed too, along with the engine effect itself (§9).

## 2. Routing a line to the horn parser vs. the existing light/drive paths

**Decision**: In `main.cpp`'s dispatch, try `parseHornCommand()` alongside the existing
`parseLightCommand()` check (order between the two doesn't matter — `HORN` and the eight light
words are disjoint) before falling through to `DriveCommandAssembler::apply()`, exactly matching
the existing "light-first, then drive" fallthrough pattern from `006`.

**Rationale**: `parseHornCommand()` only recognizes `HORN`, so every existing drive word, light
word, and numeric pair is already `Malformed` to it and falls through with zero behavior change
to existing commands — same reasoning as `006`'s research.md §3.

## 3. Where the "is the horn currently playing" decision lives

**Decision**: A new `ToneControl` (Control layer, pure): `apply(bool hornBusy) -> bool`,
returning `true` only when `hornBusy` is `false` — i.e., a trigger is honored only when the horn
isn't currently playing, and ignored otherwise (spec FR-005/FR-006). `hornBusy` comes from the
Hardware layer's `IToneOutput::hornBusy()` (see §5) — Control makes the decision, Hardware reports
the timing fact.

**Rationale**: This mirrors `LightsControl`'s shape (`apply()` returns whether the command
actually took effect) and keeps the "ignore while busy" rule as pure, host-testable logic with no
I/O — exactly Constitution Principle II/III's intent. Whether the horn is "currently playing" is
fundamentally a hardware-timing fact (like `MotorServoVehicleOutput`'s turn-pulse-duration
tracking), so `hornBusy()` is reported by Hardware, not computed by Control from wall-clock time
itself — Control only branches on the boolean.

**Alternatives considered**: Tracking "busy" inside `ToneControl` itself using elapsed time
(`nowMs` + the horn's fixed duration) — rejected because it duplicates a timing fact the Hardware
layer already owns to actually drive playback; a single source of truth (Hardware reports
`hornBusy()`) is simpler (Principle I).

## 4. Only the horn has a busy concept

**Decision**: `IToneOutput` exposes a single busy flag, `hornBusy()`, and `ToneControl` is only
consulted for the horn. There is no per-effect or global busy tracking.

**Rationale**: With the horn as the only effect (§9), "is anything playing" and "is the horn
playing" are the same question, so the simplest model that satisfies FR-005/FR-006 is a single
horn-scoped flag (Principle I). Earlier designs tracked one global busy flag across horn and
engine; that only made sense while `ENGINE` was a competing manual trigger.

## 5. Audio output hardware path — single GPIO, PWM tone generation (revised)

**Decision**: Drive the car's speaker/buzzer from a **single GPIO** (`config::kToneOutputPin` =
26) using the ESP32's built-in LEDC peripheral in tone-generation mode (the same PWM hardware
`PwmSteeringServo` already uses for servo pulses, repurposed here for audible-frequency square
waves) — not an external I2S DAC/amplifier module, and not the built-in analog DAC.

**Rationale**: Per direct instruction, horn playback is wired through GPIO26 specifically. A
single square-wave-capable GPIO driving a small speaker/piezo buzzer (optionally through a simple
transistor driver stage) is the simplest possible way to produce an audible tone, needs no
external DAC/amp module or multi-pin I2S bus, and avoids the `kLight4Pin` (GPIO25) conflict the
built-in analog DAC would have caused. This also supersedes and simplifies the earlier draft's
three-pin external-I2S-DAC research (§7 in the prior revision of this document) — one pin
replaces three, and no audio codec hardware is needed at all, just a speaker/buzzer.

**Alternatives considered**: External I2S DAC/amp (the previous decision in this document) —
superseded; it added hardware (a codec module) and three pins for a fidelity level this feature
does not need, since every effect is a synthesized tone/rumble, not high-fidelity recorded audio.
Built-in analog DAC (GPIO25/26) — still rejected for the `kLight4Pin` conflict, and moot anyway
since LEDC tone generation on GPIO26 doesn't use the DAC peripheral at all.

## 6. Local tone generation: synthesized waveform, not a decoded audio file

**Decision**: Each sound effect is a small, fixed waveform **synthesized in firmware** via LEDC
tone generation (frequency, and where needed frequency/gating modulation over time) — not a
decoded audio file. No audio file format (WAV/MP3), no decoder, and no asset pipeline is
introduced.

**Rationale**: "An already-conceived tone" only requires the sound to be fixed and pre-decided,
not that it be a recorded sample. A single LEDC channel can only produce one frequency (a square
wave) at an instant, so per-effect "character" comes from how that frequency is varied over the
effect's duration, not from mixing waveforms — this keeps the feature to plain arithmetic plus
`ledcWriteTone()`-style calls (Principle I), matching the single-GPIO hardware choice in §5.

**Alternatives considered**: Storing a recorded PCM/WAV sample per effect in flash and playing it
back through a DAC — not pursued; it would need the codec hardware/pins rejected in §5 and an
audio-asset pipeline this project doesn't have, for a fidelity level not required by the spec.

### Per-effect waveform design

- **Horn** (`config::kHornDurationMs` = 1500, per explicit instruction): a fixed, constant
  `config::kHornFreqHz` (420Hz) tone held for the full 1500ms, then stops. Simple, constant,
  unmistakable.
- **Engine** (removed — see §9): had been an automatic V8-style idle (~52Hz) / running (~147Hz)
  rumble derived from firing-frequency math, wobbled around its base frequency. Removed after
  on-device testing.

**Siren** (a classic rising/falling frequency sweep) was designed and briefly implemented but
removed before shipping — the car is not an ambulance, so it didn't fit. The `SIREN` word is no
longer recognized (`Malformed`).

## 7. Non-blocking playback — satisfying "never delay drive commands"

**Decision**: The real `IToneOutput` implementation starts the horn tone with a single
`ledcWriteTone()` in `playHorn()` and stops it from a `tick(unsigned long nowMs)` method called
once per `main.cpp` `loop()` iteration (same call shape as `MotorServoVehicleOutput::tick(millis())`)
once the horn's duration has elapsed — every call is O(1), never a blocking wait for the horn's
duration. `hornBusy()` reflects whether the horn is still within its `[hornStartMs, hornStartMs +
kHornDurationMs)` window.

**Rationale**: Unlike the earlier A2DP design (where a vendor library ran on its own FreeRTOS
task), sound-effect playback now happens entirely on the same Arduino main task as drive-command
handling — so "never delay drive commands" (FR-007) must be achieved by design, not by task
isolation. Since LEDC tone generation is hardware-timer-driven (the ESP32 keeps outputting the
configured square wave without CPU involvement between `tick()` calls), `tick()` only needs to
decide *when to change* the frequency, which is cheap integer arithmetic — the same technique
`MotorServoVehicleOutput` already uses for the turn-pulse auto-center timer.

**Alternatives considered**: Bit-banging the waveform in software (manually toggling the GPIO in
a tight loop) — rejected outright; this would block `loop()`, directly violating FR-007. LEDC
avoids this because the hardware peripheral, not the CPU, generates the waveform once configured.

## 9. The engine tone was tried and removed

**Decision**: An automatic engine tone was implemented and then removed after on-device testing;
only the horn remains, and the output is silent whenever the horn isn't playing. Removed:
`IToneOutput::setEngineRunning()`, `motorEngaged(Direction)` (in `control/direction.h`), the
`kEngine*` and `kToneLayer*` constants, the wobble/layering logic in `PwmToneOutput`, and
`test_drive_to_engine_tone_flow.cpp`.

**History**: The engine had first been a manual `ENGINE` trigger word, then became fully automatic —
idle (~52Hz, from V8 firing-frequency math at ~775 RPM) whenever the DC motor wasn't engaged and a
higher running tone (~147Hz) whenever it was, chosen from the already-decided `Direction` via
`motorEngaged()` — with the horn layered on top by time-slicing the single LEDC channel between
the horn and engine frequencies (100ms window, 60ms horn). None of that survived on-device
testing, and the horn stands on its own (Principle I).

**Consequences**: `PwmToneOutput` no longer has any always-on output — `begin()` leaves the
channel silent, `playHorn()` writes the horn frequency, and `tick()` writes 0 once the horn's
duration elapses. `main.cpp` no longer touches the tone output when drive commands are handled.

## 8. No new third-party dependency

**Decision**: Remove the `pschatzmann/ESP32-A2DP` dependency added under the earlier design.
Tone generation uses only the ESP32 Arduino core's built-in LEDC API — confirmed by an actual
`pio run -e esp32dev` build against the pinned platform (`framework-arduinoespressif32 @
3.20017.241212`, a 2.x-generation core) to be the channel-based form (`ledcSetup(channel, freq,
resolution)` + `ledcAttachPin(pin, channel)` once in `begin()`, then `ledcWriteTone(channel,
freq)`/`ledcWrite(channel, 0)` from `tick()`), not the newer pin-based `ledcAttach()` form some
other ESP32 core versions use. `config::kToneLedcChannel` (15) is reserved separately from
whatever channel `ESP32Servo` auto-allocates for the steering servo. Same peripheral family
`PwmSteeringServo` already uses — no new library needed.

**Rationale**: The entire reason a third-party A2DP library was needed (SBC decode + AVDTP stream
negotiation) no longer applies — there is no Bluetooth audio protocol involved at all now, only a
locally generated square wave on one GPIO using a peripheral this codebase already depends on for
the steering servo.
