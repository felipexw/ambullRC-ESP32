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
and, later, `ENGINE` (the engine sound became fully automatic — see §9 — so it no longer has a
manual trigger word at all).

**Rationale**: Same reasoning as `006`'s light commands (research.md §1 there): the app already
sends single, self-contained words per button press, and a one-shot trigger is naturally the same
shape. Unlike lights, there's no ON/OFF pair — pressing the horn button is a single event, not a
state toggle, so one word is sufficient.

**Alternatives considered**: A single generic word plus an argument (`SOUND HORN` / `SFX1`) —
rejected as an unnecessary indirection; a fixed, self-describing word is simpler and matches the
existing per-button-press wire shape exactly. `SIREN` — removed after initial implementation; not
fitting the vehicle. `ENGINE` as a manual trigger — also removed after initial implementation,
once the engine sound was redesigned to automatically track the DC motor's state instead (§9);
keeping a manual `ENGINE` word alongside the automatic behavior was considered and rejected as
redundant complexity with no clear meaning (what would triggering it do while the DC motor is
already driving?).

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

## 4. The engine has no "busy" concept at all

**Decision**: Unlike the horn, the engine tone never has a busy/idle-trigger distinction — it
always plays, and the only thing that changes is which of two frequencies is currently selected
(§9). There is no `engineBusy()` on `IToneOutput`, and `ToneControl` is only ever consulted for
the horn.

**Rationale**: The earlier design's "one global busy flag covering all effects" (superseded)
existed only because `ENGINE` was, at the time, also a manual one-shot trigger competing with
`HORN` for the same "is anything playing" gate. Once the engine sound became automatic and
continuous (§9), that gate no longer has a second command to arbitrate against, so it collapses
to "is the horn currently playing" — the simplest model that still satisfies FR-005/FR-006
(Principle I).

**Alternatives considered**: Keeping a shared busy flag between horn and engine, with the engine
"claiming" it while running — rejected; it would make the horn permanently unplayable while
driving (the engine is *always* running in one state or the other), directly contradicting the
explicit "horn plays on top of the engine" requirement (spec FR-010).

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
- **Engine**: designed to read as a lumpy V8 rather than a flat beep, in *two* states rather than
  one fixed-duration effect (see §9 for why it's automatic and continuous rather than triggered).
  A four-stroke engine fires `cylinders / 2` times per crank revolution, so firing frequency (Hz)
  = `(RPM / 60) × (cylinders / 2)`:
  - **Idle**: a V8 at a typical idle of ~750–800 RPM (in the range Mustang V8s idle at) fires at
    `(775/60) × 4 ≈ 52 Hz` (`config::kEngineIdleBaseFreqHz`) — the deep, slightly uneven
    rumble/"burble" characteristic of a V8 idle.
  - **Running**: a V8 under light throttle at a cruising ~2200 RPM fires at `(2200/60) × 4 ≈ 147
    Hz` (`config::kEngineRunningBaseFreqHz`) — deliberately much higher and, with a wider wobble
    band (`config::kEngineRunningWobbleFreqHz` = 15Hz vs. the idle's 6Hz), rougher, so switching
    between the two is unmistakable even without seeing the car move.

  Both states continuously wobble their base frequency by a few Hz on a short, irregular cycle
  (rather than holding a perfectly steady tone) to approximate that lopey character — see §9 for
  how the switch between them is driven and §10 for how the horn layers on top of whichever one is
  currently playing.

A third effect, **siren** (a classic rising/falling frequency sweep), was designed and briefly
implemented but removed before shipping — the car is not an ambulance, so it didn't fit. The
`SIREN` word is no longer recognized (`Malformed`).

This is a firmware-level approximation for a hobby project, not an attempt at high-fidelity engine
audio — it uses only well-established real V8 firing-frequency math (source: firing frequency =
(RPM/60) × cylinders/2, typical Mustang V8 idle ≈ 750–850 RPM, and a representative light-throttle
cruise RPM) to ground the frequency choices, not a recorded sample.

## 7. Non-blocking playback — satisfying "never delay drive commands"

**Decision**: The real `IToneOutput` implementation updates the LEDC tone frequency (and tracks
the horn's elapsed/remaining duration) from a `tick(unsigned long nowMs)` method called once per
`main.cpp` `loop()` iteration (same call shape as `MotorServoVehicleOutput::tick(millis())`) —
every call is O(1) (at most one `ledcWriteTone()` call to change frequency), never a blocking wait
for the horn's duration to elapse. `hornBusy()` reflects whether `nowMs` is still within the
horn's `[hornStartMs, hornStartMs + kHornDurationMs)` window; the engine has no such window since
it never stops (§4).

**Rationale**: Unlike the earlier A2DP design (where a vendor library ran on its own FreeRTOS
task), sound-effect playback now happens entirely on the same Arduino main task as drive-command
handling — so "never delay drive commands" (FR-011) must be achieved by design, not by task
isolation. Since LEDC tone generation is hardware-timer-driven (the ESP32 keeps outputting the
configured square wave without CPU involvement between `tick()` calls), `tick()` only needs to
decide *when to change* the frequency, which is cheap integer arithmetic — the same technique
`MotorServoVehicleOutput` already uses for the turn-pulse auto-center timer.

**Alternatives considered**: Bit-banging the waveform in software (manually toggling the GPIO in
a tight loop) — rejected outright; this would block `loop()`, directly violating FR-011. LEDC
avoids this because the hardware peripheral, not the CPU, generates the waveform once configured.

## 9. The engine tone is automatic, driven by `motorEngaged(Direction)`

**Decision**: `IToneOutput` gains `setEngineRunning(bool running)`, called from `main.cpp`
every time a drive command is decided (and on the safe-state STOP transition), with the boolean
computed by a new pure helper `motorEngaged(Direction)` in `control/direction.h`:
`Forward`/`Backward` and their diagonals → `true`; `Stop`/`Left`/`Right` → `false`. There is no
`ENGINE` wire command anymore (§1) — the engine tone reacts to the same `Direction` the DC
motor/servo hardware already reacts to, one call site further.

**Rationale**: The user's explicit requirement was that the engine sound reflect what the DC
motor is *actually doing*, not a separate manual "sound effect" concept — "when the dc motor...
its not working, it should play a stopped v8 car like; ... when its working (ahead or back
direction), it should play a different sound." `Direction` is already the Control layer's
decided, hardware-agnostic representation of vehicle state (spec `001`/`002`), so reusing it (via
one small pure mapping function) avoids introducing a second, parallel notion of "is the car
driving" that could drift out of sync with what `MotorServoVehicleOutput` is actually doing.
Steering alone (`Left`/`Right`, no throttle) intentionally reads as idle, since the DC motor
itself isn't engaged even though the servo is moving — this mirrors `decideDirection()`'s own
throttle-vs-steer split (`control/direction_control.cpp`).

**Alternatives considered**: Reading `DriveCommand.throttle` directly in `main.cpp` instead of
going through `Direction`/`motorEngaged()` — rejected; `Direction` already encapsulates exactly
"what the vehicle is currently doing" for every other output (log, motor, servo), so branching on
raw throttle here would be a second, redundant source of truth. A dedicated `EngineControl`
class mirroring `ToneControl`'s shape — rejected as unnecessary indirection; `motorEngaged()` is a
pure, already-testable one-line mapping with no state of its own, so a class adds a boundary with
nothing to guard (Principle I).

## 10. Horn-over-engine layering via time-slicing one LEDC channel

**Decision**: While the horn is active, `PwmToneOutput::tick()` does not silence or replace the
engine tone — it rapidly alternates the single LEDC channel between the horn frequency and
whatever engine frequency (idle or running) would otherwise be playing, within each
`config::kToneLayerPeriodMs` (100ms) window: the first `config::kToneLayerHornSliceMs` (60ms)
plays the horn, the remaining 40ms plays the engine. This repeats for the horn's whole 1500ms.

**Rationale**: The user was explicit that "when horn command is given, it should play on top of
the engine sound" (spec FR-010) — the engine must stay audible, not be interrupted by the horn.
The hardware has exactly one GPIO/LEDC channel driving one speaker (§5), which can only output one
square-wave frequency at any instant, so genuinely simultaneous mixing isn't possible without
additional hardware (a second output pin/speaker, or an external analog mixing circuit) that
wasn't asked for. Fast time-slicing (10 complete cycles/second, horn-weighted) is the standard
low-cost trick for making a single PWM/tone channel sound "layered" rather than fully replaced —
it's audibly closer to overlapping tones than either a hard cut to the horn or a hard cut back to
the engine would be, at zero additional hardware cost (Principle I).

**Alternatives considered**: A second GPIO/LEDC channel and a second speaker dedicated to the
engine, mixed with the horn's GPIO26 output in a simple external summing circuit — this would
give true simultaneous playback and was explicitly discussed, but rejected in favor of the
single-pin approach to avoid a hardware change (new pin, new wiring, updated README pinout) for a
feature that doesn't strictly require it. Silencing the engine while the horn plays (the
straightforward one-channel approach) — rejected outright; it directly contradicts spec FR-010.
Muting the horn while the engine plays — also rejected; it would make the horn unusable while
driving, since the engine is always in one state or the other.

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
