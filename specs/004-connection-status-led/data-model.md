# Data Model: Connection Status LED

## ConnectionEvent (reused, unchanged)

Produced by the existing `Control` layer (`ConnectionMonitor`, see
`specs/001-bluetooth-motor-control`/`specs/003-direction-interlock-guard`) — this feature adds no
new values and no new producer:

```
None | Connected | Disconnected
```

`LedConnectionOutput` is a second consumer of this event, alongside the existing
`SerialConnectionOutput`. Both are only ever called with `Connected` or `Disconnected` — `main.cpp`
already filters out `None` before dispatching to any `IConnectionOutput` (see `research.md` §1 for
why `None` never reaches a consumer).

## LED Signal (new, internal to the Hardware layer)

The GPIO output level driving the external LED, as understood by `LedConnectionOutput`. Never
exposed outside the Hardware layer — `Control` and `Protocol` have no knowledge of it, only of
`ConnectionEvent`.

```
OFF (LOW / 0V) | ON (HIGH / 3.3V)
```

Derived 1:1 from `ConnectionEvent` per `contracts/connection-status-led-signal-contract.md`. There
is no intermediate or transitional state — the mapping is a direct, unconditional level write, with
no timers, debouncing, or blinking (per spec Assumptions).

### State Transitions

```
       (boot / power-on)
              │
              ▼
      LED = OFF (default, FR-003)
              │
              │  ConnectionEvent::Connected
              ▼
        LED = ON  ──────────────┐
              │                  │ ConnectionEvent::Connected (already ON — no-op)
              │ ConnectionEvent::Disconnected
              ▼                  │
        LED = OFF  ◄─────────────┘
              │
              │ ConnectionEvent::Disconnected (already OFF — no-op)
              ▼
        LED = OFF
```

This mirrors `ConnectionMonitor`'s own edge-detection: because `ConnectionEvent` is only emitted on
an actual transition (never repeated while the state is unchanged), `LedConnectionOutput` never
needs to track "is it already ON/OFF" itself — every `emit()` call it receives already represents a
real change.

## Vehicle State (extends `001-bluetooth-motor-control`'s entity)

| Field | Type | Notes |
|-------|------|-------|
| `connectionLed` | LED Signal (`OFF`/`ON`) | New: the external connection-status LED's actual (applied) output level, mirroring the latest `ConnectionEvent`. |
