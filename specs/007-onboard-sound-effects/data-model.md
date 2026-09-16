# Data Model: Onboard Sound Effects

## Horn command (Protocol layer)

The horn is the only remaining manual trigger. There's no per-effect payload to carry anymore
(no `SoundEffectId`/`ToneCommand` — that only made sense while `ENGINE` was also a manual word),
so parsing collapses to a single recognized word:

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

## motorEngaged() (Control layer, `control/direction.h`)

```cpp
bool motorEngaged(Direction direction);
```

A pure mapping from the already-decided `Direction` (spec `001`/`002`) to whether the DC motor is
actually engaged: `true` for `Forward`/`Backward` and their diagonals; `false` for `Stop`,
`Left`, and `Right` (steering alone doesn't move the DC motor). `main.cpp` calls this every time a
drive command is decided (and on the safe-state STOP transition) to drive
`IToneOutput::setEngineRunning()` — this is what makes the engine sound automatic (spec FR-007).

## IToneOutput (Hardware layer)

```cpp
class IToneOutput {
 public:
  virtual ~IToneOutput() = default;
  virtual void playHorn() = 0;             // starts the horn; a no-op while already playing
  virtual bool hornBusy() = 0;             // true while the horn is still playing
  virtual void setEngineRunning(bool running) = 0;  // selects idle vs. running engine tone
  virtual void tick(unsigned long nowMs) = 0;       // advances playback; called once per loop()
};
```

- Real implementation: `PwmToneOutput` (ESP32-only) — drives `config::kToneOutputPin` via the
  LEDC tone-generation peripheral. The engine tone plays continuously (idle or running,
  whichever `setEngineRunning()` last selected); the horn, while active, is layered on top by
  time-slicing the same LEDC channel between the horn frequency and the current engine frequency
  (research.md §5–§7). Validated on-device via `quickstart.md`, not a host unit test (same
  precedent as `PwmSteeringServo`/`GpioMotorDriver`/`GpioLightsOutput`).
- Test double: `FakeToneOutput` — records every `playHorn()` call (and sets `hornBusy()` true, as
  the real device does), records every `setEngineRunning()` call (and its current value), and
  lets tests set `hornBusy()` directly to simulate "still playing" without real timing.

## What is explicitly *not* modeled here

- **Audio waveform data itself**: not a data-model concern — it's a Hardware-layer implementation
  detail of `PwmToneOutput` (research.md §6), never inspected or decided by Protocol/Control.
- **Volume**: not modeled; fixed and hardware-controlled (spec FR-014).
- **True simultaneous horn+engine audio mixing**: deliberately out of scope — one GPIO/LEDC
  channel time-slices between the two frequencies as an approximation (spec Assumptions).
- **A "busy" concept for the engine**: the engine never stops, so there's nothing to be busy with;
  only the horn has a busy/idle distinction.
