# Data Model: Four-Light Auxiliary Control

## LightCommand (new, Protocol layer)

A single, already-validated instruction to switch one of the four lights on or off, produced by
`parseLightCommand()` only on `ParseResult::Ok`.

| Field | Type | Notes |
|-------|------|-------|
| `lightId` | int | `1`–`4` (1-based, matches the wire word `LIGHT<n>...` and the report bit position). Never outside this range on `Ok` — `parseLightCommand()` returns `OutOfRange` instead of a `LightCommand` for `LIGHT5ON` etc. |
| `on` | bool | `true` = switch this light ON, `false` = switch it OFF. |

## LightsState (new, Protocol layer)

```
using LightsState = std::array<bool, config::kLightCount>;  // kLightCount = 4
```

The current ON/OFF state of all four lights together, indexed `0..3` for lights `1..4`
(`state[0]` is light 1, etc.). This is the one type both `LightsControl` (Control) and
`formatLightsReport()`/`GpioLightsOutput` (Protocol/Hardware) operate on — see research.md §5 for
why it lives in Protocol rather than Control.

- All-`false` is the boot-time default (FR-006) — no persisted state exists before the first
  command.
- Independent per index: switching one light never changes any other index (FR-003).

## LightsControl (new, Control layer)

Pure decision logic, no I/O: applies a validated `LightCommand` to the persisted `LightsState` and
reports whether anything actually changed.

```
class LightsControl {
 public:
  // Applies `command` to the persisted state, writes the full resulting
  // state to `outState`, and returns true iff outState differs from the
  // state before this call (i.e., a report should be sent per FR-004).
  // Setting a light to the state it's already in returns false and leaves
  // outState equal to the unchanged state (still written, for convenience).
  bool apply(const LightCommand& command, LightsState& outState);

  // The current state, e.g. for a full resync report on connect (FR-005).
  LightsState state() const;

 private:
  LightsState state_{};  // all false — FR-006
};
```

### State Transitions

Each of the 4 lights is an independent 2-state machine (`OFF ⇄ ON`), driven only by a
`LightCommand` naming that light's `lightId`:

```
        (boot / power-on)
               │
               ▼
       light[n] = OFF (FR-006, all 4)
               │
               │ LightCommand{lightId=n, on=true}
               ▼
       light[n] = ON  ──────────────┐
               │                     │ LightCommand{lightId=n, on=true} (already ON — no-op, no report)
               │ LightCommand{lightId=n, on=false}
               ▼                     │
       light[n] = OFF ◄──────────────┘
               │
               │ LightCommand{lightId=n, on=false} (already OFF — no-op, no report)
               ▼
       light[n] = OFF
```

A Bluetooth disconnect/reconnect does **not** transition any light (spec FR-009 / Assumptions) —
unlike `Direction`'s fail-safe state machine, there is no disconnect-triggered edge here. The only
external trigger that causes a report without a state transition is a new connection (FR-005),
which reads the current state rather than changing it.

## `LightsState` → wire report (Protocol layer)

`formatLightsReport(const LightsState& state) -> std::string`, per
`contracts/light-command-protocol.md`:

```
"LIGHTS:" + (state[0] ? '1' : '0') + (state[1] ? '1' : '0') + (state[2] ? '1' : '0') + (state[3] ? '1' : '0')
```

## `LightsState` → GPIO (Hardware layer)

`ILightsOutput::apply(const LightsState& state)`, real implementation `GpioLightsOutput`, per
`contracts/lights-state-to-gpio-contract.md`: writes each `state[i]` to its configured pin
(`config::kLight1Pin`..`kLight4Pin`), `HIGH` for `true` (ON), `LOW` for `false` (OFF).

## Vehicle/session state (extends existing entities)

| Field | Type | Notes |
|-------|------|-------|
| `lights` | `LightsState` | New: the four auxiliary lights' current commanded state. Unlike `connectionLed` (`004`), this state is not derived from `ConnectionEvent` — it's derived only from `LightCommand`s and defaults OFF at boot. |
