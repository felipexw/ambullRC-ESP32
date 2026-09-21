# Data Model: Onboard Sound Effects

## Horn command (Protocol layer)

The horn is the only sound effect. There's no per-effect payload to carry (no
`SoundEffectId`/`ToneCommand` — that only made sense while there were several effects), so parsing
is a single recognized word:

```cpp
ParseResult parseHornCommand(const std::string& line);
```

`"HORN"` (case-insensitive) → `Ok`; anything else — including the now-removed `"SIREN"` and
`"ENGINE"` words — → `Malformed` (falls through to the existing light/drive parsers per
research.md §2).

## ToneControl (Control layer)

Pure decision logic, no I/O: decides whether a recognized horn trigger should actually start
playback, given whether the horn is currently busy (spec FR-005/FR-006).

```cpp
class ToneControl {
 public:
  // Returns true iff hornBusy is false — the request is honored only when
  // the horn isn't already playing. Returns false (ignoring the trigger) if
  // hornBusy is true.
  bool apply(bool hornBusy);
};
```

No persisted state of its own — `hornBusy` comes from `IToneOutput::hornBusy()` each call. Kept
as a class (rather than a free function) to match the existing `LightsControl`/`DirectionControl`
shape used elsewhere in `Control`, even though it's now a single-line decision.

### Decision table

| `hornBusy` | Result |
|---|---|
| `false` | `apply()` returns `true`; caller triggers `IToneOutput::playHorn()` |
| `true` | `apply()` returns `false`; nothing is triggered (spec FR-005) |

## IToneOutput (Hardware layer)

```cpp
class IToneOutput {
 public:
  virtual ~IToneOutput() = default;
  virtual void playHorn() = 0;             // starts the horn; a no-op while already playing
  virtual bool hornBusy() = 0;             // true while the horn is still playing
  virtual void tick(unsigned long nowMs) = 0;       // ends the horn once its duration elapses;
                                                    // called once per loop()
};
```

- Real implementation: `PwmToneOutput` (ESP32-only) — drives `config::kToneOutputPin` via the
  LEDC tone-generation peripheral: silent at boot, `config::kHornFreqHz` for
  `config::kHornDurationMs` after `playHorn()`, then silent again (research.md §5–§7). Validated
  on-device via `quickstart.md`, not a host unit test (same precedent as `PwmSteeringServo`/
  `GpioMotorDriver`/`GpioLightsOutput`).
- Test double: `FakeToneOutput` — records every `playHorn()` call (and sets `hornBusy()` true, as
  the real device does), and lets tests set `hornBusy()` directly to simulate "still playing"
  without real timing.

## What is explicitly *not* modeled here

- **Audio waveform data itself**: not a data-model concern — it's a Hardware-layer implementation
  detail of `PwmToneOutput` (research.md §6), never inspected or decided by Protocol/Control.
- **Volume**: not modeled; fixed and hardware-controlled (spec FR-010).
