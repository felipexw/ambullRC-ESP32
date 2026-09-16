# Tone Output Hardware Contract

Satisfies FR-002, FR-003, FR-007, FR-008, FR-009, FR-010, FR-011, FR-012, FR-014. This is the
contract for `PwmToneOutput`, the real `IToneOutput` implementation (Hardware layer).

## Output pin

| Signal | Config value |
|---|---|
| Tone output (to speaker/buzzer, via a simple driver stage if needed) | `config::kToneOutputPin` = 26 |

A single GPIO driven via the ESP32's LEDC tone-generation peripheral (research.md §5) — not an
external I2S DAC/amp, not the built-in analog DAC. Volume is fixed, set by a hardware trim
potentiometer downstream of this pin; no firmware code controls it (FR-014).

## Engine tone: continuous, automatic (research.md §6)

Unlike the horn, the engine tone is never "started" or "stopped" by a command — it always plays,
and `setEngineRunning(bool)` only selects which of two continuously-wobbling frequencies is
currently active:

| State | Base frequency | Wobble | When it plays |
|---|---|---|---|
| Idle | `config::kEngineIdleBaseFreqHz` (52Hz, ≈ a Mustang-like ~775 RPM V8 idle firing frequency) | ±`config::kEngineIdleWobbleFreqHz` (6Hz) | Whenever `setEngineRunning(false)` was last called — DC motor not engaged (FR-008) |
| Running | `config::kEngineRunningBaseFreqHz` (147Hz, ≈ a ~2200 RPM light-throttle V8 firing frequency) | ±`config::kEngineRunningWobbleFreqHz` (15Hz) | Whenever `setEngineRunning(true)` was last called — DC motor engaged, forward or backward (FR-009) |

Both wobble on the same short, irregular two-cycle pattern used previously for the idle-only
design, just around a different base/wobble amplitude, so "running" reads as louder/rougher, not
merely higher-pitched.

## Horn: fixed duration and tone, layered over the engine (research.md §6, §7)

| Effect | Duration | Frequency |
|---|---|---|
| Horn | `config::kHornDurationMs` = 1500ms | Constant `config::kHornFreqHz` (420Hz) |

`playHorn()` records a start timestamp and sets the horn active. While active, `tick(nowMs)` does
**not** silence the engine tone — since a single LEDC channel can only output one frequency at an
instant, the horn is layered "on top" (FR-010) by rapidly time-slicing within each
`config::kToneLayerPeriodMs` (100ms) window: the first `config::kToneLayerHornSliceMs` (60ms) of
each window plays the horn frequency, the remainder plays whatever the current engine frequency
(idle or running) would otherwise be. This repeats for the horn's full 1500ms, then the horn
frequency drops out and the engine tone plays uninterrupted again. This is a deliberate
approximation given the single-GPIO/single-speaker hardware (spec Assumptions), not true
simultaneous audio mixing.

## Non-blocking playback (FR-011, FR-012)

`tick(nowMs)` is O(1) per call: it checks whether the horn's `[hornStartMs, hornStartMs +
kHornDurationMs)` window has elapsed (clearing the horn-active flag if so), computes the
appropriate frequency per the tables above, and calls the LEDC tone-frequency update — never a
blocking wait. This is what makes FR-011 ("never delay drive commands") and FR-012 ("never cut
off a playing horn because of a drive command") true simultaneously: neither path blocks the
other, because neither path blocks at all — the LEDC hardware peripheral keeps generating the
configured square wave between `tick()` calls with no CPU involvement.

## `hornBusy()` semantics

`hornBusy()` returns `true` from the moment `playHorn()` starts until `tick()` observes `nowMs`
has passed the horn's duration window, then `false` again. There is no equivalent "busy" concept
for the engine tone — it never stops, so `IToneOutput` has no `engineBusy()`.

## Boot-time default

At boot, the engine tone starts immediately in its **idle** state (`engineRunning_` defaults to
`false`) — not silent. This differs from the horn's boot default (`hornBusy()` is `false`, no
horn playing) and from the previous, now-superseded design where the whole tone output was silent
until a trigger arrived. A car that idles from the moment it's powered on, before anyone drives
it, is the intended behavior (spec Assumptions).

## Out of scope

- The physical speaker/buzzer and its volume trimpot wiring (spec Assumptions).
- Any outgoing report or acknowledgement to the app (`tone-command-protocol.md`'s "Out of
  scope").
- Interaction with the DC-motor/servo fail-safe path — sound effects are not part of Constitution
  Principle V's scope, mirroring the auxiliary-lights precedent (spec FR-013, Assumptions). The
  engine sound does return to idle on a safe-state transition, but only as a side effect of
  `motorEngaged()` reading `Direction::Stop`, not because of any new fail-safe logic added here.
- True simultaneous horn+engine audio mixing (spec Assumptions) — the time-slicing approach above
  is an approximation, not a claim of genuinely overlapping waveforms.
