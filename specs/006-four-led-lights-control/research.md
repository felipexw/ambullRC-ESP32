# Phase 0 Research: Four-Light Auxiliary Control

## 1. Wire format for the incoming per-light command

**Decision**: Extend the existing word-command vocabulary (`UP`/`DOWN`/`LEFT`/`RIGHT`/`STOP`/
`CENTER`) with eight new words of the same shape: `LIGHT1ON`, `LIGHT1OFF`, `LIGHT2ON`,
`LIGHT2OFF`, `LIGHT3ON`, `LIGHT3OFF`, `LIGHT4ON`, `LIGHT4OFF` — case-insensitive, no delimiters,
one per line, parsed by a new `parseLightCommand()` that reuses the existing `ParseResult`
enum (`Ok`/`Malformed`/`OutOfRange`).

**Rationale**: The app already sends single, self-contained words per button press
(`bluetooth-command-protocol.md`); a light toggle is naturally the same shape (one button → one
word), so no new delimiter syntax (e.g. `LIGHT 1 ON`) or wire form has to be introduced. Keeping
the light number embedded in the word itself (`LIGHT<n>ON/OFF`) lets `parseLightCommand()` reject
an unrecognized light number (`LIGHT5ON` → `OutOfRange`) using the same category the numeric
drive command already uses for a steer/throttle value outside range, instead of inventing a new
result type.

**Alternatives considered**: A single word plus a separate numeric argument
(`LIGHT 1,ON` or `1,LIGHTON`) — rejected as an unnecessary second delimiter style alongside the
two the protocol already has (bare words, comma-separated numeric pair); it would also complicate
routing (see §3) because a line could no longer be classified by trying one parser and falling
through to the other.

## 2. Wire format for the outgoing state report

**Decision**: A single combined line, `LIGHTS:` followed by exactly 4 characters, one per light in
order (`1` = ON, `0` = OFF), e.g. `LIGHTS:1001` (light 1 ON, 2 and 3 OFF, 4 ON). The ESP32 sends
this line both when any light's state changes as a result of a command (spec FR-004) and whenever
a new Bluetooth connection is established (spec FR-005) — the same line shape covers both cases.

**Rationale**: A combined report is simpler than four independent per-light report lines
(`LIGHT1:ON`, `LIGHT2:OFF`, ...) because it gives the app one atomic snapshot to parse and apply,
with no risk of the app's UI ending up with a stale value for a light that wasn't part of the
triggering event. It also means User Story 2 (report after a change) and User Story 3 (report on
reconnect) are the exact same message, reusing one formatter (`formatLightsReport()`) instead of
two.

**Alternatives considered**: Four independent `LIGHT<n>:<ON|OFF>` lines — rejected because full
resync (FR-005) would then require sending four lines instead of one, and the app would need to
merge them into a picture of "all lights" itself rather than receiving one already-complete
snapshot.

## 3. Routing a line to the light parser vs. the existing drive-command assembler

**Decision**: On each received line, try `parseLightCommand()` first. If it returns `Ok`, treat the
line as a light command (apply it, skip drive-command parsing for that line). Otherwise
(`Malformed` or `OutOfRange`), fall through unchanged to the existing
`DriveCommandAssembler::apply()` path, exactly as today.

**Rationale**: `parseLightCommand()` only recognizes the fixed `LIGHT<n>ON`/`LIGHT<n>OFF` shape, so
every existing drive word (`UP`, `LEFT`, ...) and numeric pair (`0,0`) is already `Malformed` to
it and falls through with zero behavior change to existing commands. This keeps the two command
families fully decoupled (no shared state, no new "is this a light line" pre-check needed) and
requires touching `main.cpp`'s dispatch in exactly one place.

**Alternatives considered**: A single unified parser/assembler handling both drive and light
commands — rejected as unnecessary coupling between two independent concerns (steering/throttle
vs. auxiliary lights) for no behavioral benefit, and it would make `DriveCommandAssembler`
responsible for state it doesn't otherwise need to know about.

## 4. Sending data back to the app — extending `ITransport`

**Decision**: Add one new method to `ITransport`, `writeLine(const std::string& line)`, mirroring
the existing `readLine()`. `BluetoothTransport` implements it with `BluetoothSerial::println()`;
`FakeTransport` (host tests) records every written line for assertions.

**Rationale**: Nothing in the current codebase sends data back to the app — the wire protocol has
been receive-only through `005`. `ITransport` is exactly where that capability belongs per the
Constitution's layering (Transport = "Bluetooth send/receive only"); no existing interface needs
to change shape, only grow by one method, and `main.cpp` remains the only place a decided
Control-layer value (the lights' state) is hand off to `ITransport`, the same way it already hands
off `Direction`/`ConnectionEvent` to `Hardware`-layer consumers.

**Alternatives considered**: A separate `IAppReporter`/`IStateReporter` interface distinct from
`ITransport` — rejected as speculative generality (Principle I): there is exactly one transport
(Bluetooth) and exactly one place data is sent back (this feature), so a second interface adds
indirection with no current benefit over extending the interface that already owns "talk to the
Bluetooth link."

## 5. Where `LightsState`/`LightCommand` live

**Decision**: Both types live in the `Protocol` layer (alongside `DriveCommand` in
`command_parser.h`), not `Control`. `LightsControl` (Control layer) depends on these Protocol
types the same way `DirectionControl` already depends on `protocol::DriveCommand`.

**Rationale**: The outgoing report (`formatLightsReport()`) needs to turn a `LightsState` into a
wire line, and formatting-for-the-wire is Protocol's job. If `LightsState` lived in `Control`
instead, the Protocol-layer formatter would have to depend on `Control`, inverting the
Constitution's one-way `Transport → Protocol → Control → Hardware` dependency direction. Defining
the type in `Protocol` and letting `Control` consume it (as it already does for `DriveCommand`)
keeps every dependency arrow pointing the same direction as today.

**Alternatives considered**: Duplicate types (a `Protocol::LightsState` and a separate
`Control::LightsState`) with a conversion step — rejected as pure ceremony (Principle I): the two
would always be structurally identical, so a single shared type is simpler and has nothing to get
out of sync.

## 6. GPIO pin selection for the four lights

**Decision**: Four new dedicated GPIO pins, distinct from every pin already in use
(`kServoPin`=13, `kMotorPinA`=18, `kMotorPinB`=19, `kLedPin`=12), chosen from the ESP32 Dev
Module's general-purpose output-capable pins that are not boot-strapping pins (avoiding GPIO0, 2,
5, 12, 15) and not input-only (avoiding GPIO34–39): GPIO21, 22, 23, 25.

**Rationale**: Same reasoning `004-connection-status-led`'s research already applied to
`kLedPin` — avoiding strapping pins keeps the lights' default output level from being able to
interfere with boot-mode selection, and avoiding input-only pins is required since these are
outputs. The exact numbers are an implementation-time `config.h` value, not a spec-level decision.

**Alternatives considered**: Reusing/multiplexing existing pins — rejected; the spec is explicit
about "4 leds, each one in one GPIO," and the four candidate pins are free.

## 7. `ILightsOutput` interface shape

**Decision**: One method, `apply(const LightsState& state)`, called with the full 4-light state
every time any light changes — not four independent `setLight(int id, bool on)` calls or a
per-light interface.

**Rationale**: `GpioLightsOutput::apply()` just writes all four pins from the state array
unconditionally; that's simpler than tracking a "did only light 2 change" delta and is cheap
enough (4 `digitalWrite()` calls) that no incremental-update optimization is warranted (Principle
I). It also mirrors `IVehicleOutput::emit(Direction)`, which likewise hands the Hardware layer one
complete decided value rather than a partial delta.

**Alternatives considered**: `setLight(int id, bool on)` per change — rejected as it would need
`main.cpp` to know which single light changed and call the interface once per light, when
`LightsControl::apply()` already computes the full resulting `LightsState` in one step.
