# Tone Output Hardware Contract

Satisfies FR-002, FR-003, FR-006, FR-007, FR-008, FR-010. This is the contract for
`DacToneOutput`, the real `IToneOutput` implementation (Hardware layer). It replaced the original
LEDC-based `PwmToneOutput`.

## Output pin

| Signal | Config value |
|---|---|
| Tone output (built-in DAC channel 2 → coupling cap → LM386) | `config::kToneOutputPin` = 26 |

The pin is driven by the ESP32's built-in 8-bit DAC via `dacWrite()`. It does not use I2S/DMA or
a stored sample. Volume is fixed, set by a hardware trim potentiometer downstream of this pin; no
firmware code controls it (FR-010).

## Horn: fixed duration and tone (research.md §6, §7)

| Effect | Duration | Frequency |
|---|---|---|
| Horn | `config::kHornDurationMs` = 1500ms | Constant `config::kHornFreqHz` (420Hz) |

The waveform is a square wave that alternates between two DAC levels around the silent midline
(128). `HornWaveform` (pure, host-tested) produces one level per half-period for
`kHornDurationMs`, then returns to the midline.

## Silence otherwise

`begin()` writes the midline (silence) to the DAC and starts a periodic `esp_timer` at the
half-period. While the horn is idle, each timer tick yields the midline and writes nothing. The
output is silent at boot and whenever the horn is not playing (FR-006); there is no background or
idle sound.

## Non-blocking playback (FR-007, FR-008)

`playHorn()` only arms `HornWaveform` and returns immediately. The DAC writes run on the ESP-IDF
timer task, not in `loop()`, so drive and steering commands are never delayed by a playing horn
(FR-007). Drive commands never touch the tone output, so they cannot cut off a playing horn
(FR-008).

An earlier `DacToneOutput` generated the tone with a `delayMicroseconds()` loop inside
`playHorn()`. That loop blocked `loop()` for the full 1.5s, so the steering servo and DC motor
ignored commands while the horn sounded. The timer-driven design above fixes that.

## `hornBusy()` semantics

`hornBusy()` returns `true` from the moment `playHorn()` arms the tone until the last half-period
has been played, then `false` again. A retrigger while busy is ignored (FR-004).

## Out of scope

- The physical speaker/buzzer and its volume trimpot wiring (spec Assumptions).
- Any outgoing report or acknowledgement to the app (`tone-command-protocol.md`'s "Out of
  scope").
- Interaction with the DC-motor/servo fail-safe path — the horn is not part of Constitution
  Principle V's scope, mirroring the auxiliary-lights precedent (spec FR-009, Assumptions).
