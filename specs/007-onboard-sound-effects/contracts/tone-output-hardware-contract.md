# Tone Output Hardware Contract

Satisfies FR-002, FR-003, FR-006, FR-007, FR-008, FR-010. This is the contract for
`PwmToneOutput`, the real `IToneOutput` implementation (Hardware layer).

## Output pin

| Signal | Config value |
|---|---|
| Tone output (to speaker/buzzer, via a simple driver stage if needed) | `config::kToneOutputPin` = 26 |

A single GPIO driven via the ESP32's LEDC tone-generation peripheral (research.md §5) — not an
external I2S DAC/amp, not the built-in analog DAC. Volume is fixed, set by a hardware trim
potentiometer downstream of this pin; no firmware code controls it (FR-010).

## Horn: fixed duration and tone (research.md §6, §7)

| Effect | Duration | Frequency |
|---|---|---|
| Horn | `config::kHornDurationMs` = 1500ms | Constant `config::kHornFreqHz` (420Hz) |

`playHorn()` records a start timestamp, sets the horn active, and writes the horn frequency to the
LEDC channel. `tick(nowMs)` clears the horn-active flag and writes 0 (silence) once `nowMs` has
passed the `[hornStartMs, hornStartMs + kHornDurationMs)` window.

## Silence otherwise

`begin()` sets up the LEDC channel and leaves it silent. The output is silent at boot and whenever
the horn is not playing (FR-006) — there is no background or idle sound.

## Non-blocking playback (FR-007, FR-008)

`playHorn()` and `tick(nowMs)` are O(1) per call: at most one LEDC tone write, never a blocking
wait. This is what makes FR-007 ("never delay drive commands") and FR-008 ("never cut off a
playing horn because of a drive command") true simultaneously: neither path blocks the other,
because neither path blocks at all — the LEDC hardware peripheral keeps generating the configured
square wave between `tick()` calls with no CPU involvement.

## `hornBusy()` semantics

`hornBusy()` returns `true` from the moment `playHorn()` starts until `tick()` observes `nowMs`
has passed the horn's duration window, then `false` again.

## Out of scope

- The physical speaker/buzzer and its volume trimpot wiring (spec Assumptions).
- Any outgoing report or acknowledgement to the app (`tone-command-protocol.md`'s "Out of
  scope").
- Interaction with the DC-motor/servo fail-safe path — the horn is not part of Constitution
  Principle V's scope, mirroring the auxiliary-lights precedent (spec FR-009, Assumptions).
