# Feature Specification: Connection Status LED

**Feature Branch**: `004-connection-status-led`

**Created**: 2026-07-27

**Status**: Draft

**Input**: User description: "Every time there's a successfull Bluetooth connection between the ESP32 and the app, it should switch on a LED. Is out of scope the circuit, just consider sending a signal 3.3V to this LED instead. In other words, when it connects, it should switch on the LED light and it will switch off when the Blueooth connection is lost. This LED is hanging outside the RC car and its purpose is only to give feedback regarding the Bluetooth connection (paired)"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - LED lights up on successful pairing (Priority: P1)

As the operator of the RC car, when I successfully pair my app with the car over Bluetooth, I want an external LED on the car to light up, so that I have clear visual confirmation the car is ready to receive commands before I start driving.

**Why this priority**: This is the core value of the feature. Without it, the operator has no way to visually confirm the car is paired and ready, and would have to guess or rely on the app alone.

**Independent Test**: Can be fully tested by establishing a Bluetooth connection from the app to the ESP32 and observing that the LED output signal switches to ON, without needing the disconnect behavior to be implemented.

**Acceptance Scenarios**:

1. **Given** the car is powered on and no Bluetooth connection is active, **When** the app successfully connects and pairs with the ESP32, **Then** the LED output signal switches ON.
2. **Given** the LED is already ON from a prior successful connection, **When** another successful connection event occurs (e.g., reconnect), **Then** the LED remains ON (no flicker or toggle).

---

### User Story 2 - LED turns off when connection is lost (Priority: P1)

As the operator of the RC car, when the Bluetooth connection between my app and the car drops, I want the external LED to switch off, so that I immediately know the car is no longer receiving my commands and I should stop treating it as controllable.

**Why this priority**: Losing the connection indicator is as important as gaining it — an LED that stays on after disconnect would falsely signal the car is still controllable, which is a safety-relevant miscue given the car's fail-safe requirements on disconnect.

**Independent Test**: Can be fully tested by establishing a connection (LED ON), then dropping it, and observing the LED output signal switches to OFF, independent of how the ON behavior is implemented.

**Acceptance Scenarios**:

1. **Given** the LED is ON due to an active Bluetooth connection, **When** the connection is lost (app disconnects, goes out of range, or the link drops), **Then** the LED output signal switches OFF.
2. **Given** the Bluetooth connection is lost and the LED is OFF, **When** no new connection is established, **Then** the LED remains OFF.

---

### User Story 3 - LED reflects correct state on power-up and repeated cycles (Priority: P2)

As the operator, when I power on the car before pairing, I want the LED to be off by default, and I want it to correctly track multiple connect/disconnect cycles over a session, so the indicator is trustworthy every time I use the car, not just the first time.

**Why this priority**: Ensures the indicator is reliable across the full lifecycle of use (power-on, multiple pairing attempts during a session), not just a one-shot demo behavior.

**Independent Test**: Can be fully tested by power-cycling the device and observing the LED starts OFF, then repeating connect/disconnect several times in a row and confirming the LED tracks each transition correctly.

**Acceptance Scenarios**:

1. **Given** the car has just powered on and has never received a connection, **When** no Bluetooth connection has been made yet, **Then** the LED output signal is OFF.
2. **Given** a session with repeated connect/disconnect/reconnect cycles, **When** each transition occurs, **Then** the LED output signal always matches the current connection state (ON only while connected).

---

### Edge Cases

- What happens if the connection drops and reconnects in rapid succession (flapping link)? The LED MUST track each transition without getting stuck in the wrong state; it does not need to guarantee visible flicker for very fast cycles.
- What happens if the device loses power while connected? On next power-up, the LED MUST start OFF (state is not assumed to persist across power cycles) until a new successful connection is established.
- What happens if a malformed or partial connection attempt occurs (never reaches "successfully connected")? The LED MUST remain OFF; only a fully successful connection turns it ON.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST switch the LED output signal ON immediately when a Bluetooth connection between the ESP32 and the app is successfully established.
- **FR-002**: System MUST switch the LED output signal OFF immediately when the Bluetooth connection between the ESP32 and the app is lost or ends.
- **FR-003**: System MUST default the LED output signal to OFF whenever there is no active, successful Bluetooth connection (including at power-on/boot, before any connection has ever been made).
- **FR-004**: The LED output MUST only reflect the current Bluetooth connection state — it MUST NOT be used to signal any other condition (e.g., driving activity, errors, battery level).
- **FR-005**: System MUST correctly reflect repeated connect/disconnect/reconnect cycles for the lifetime of a power-on session, with no stuck or stale state.
- **FR-006**: The mechanism used to drive the LED MUST follow the same fail-safe posture as the rest of the system: on any condition that already causes the vehicle to fail safe (disconnect, timeout), the LED MUST reflect "not connected" (OFF).

### Key Entities

- **Connection Status Indicator**: A single binary output (ON/OFF) representing whether the ESP32 currently has a successful, active Bluetooth connection with the app. Physically realized as a signal to an external LED; the LED's circuit and placement are outside this feature's scope.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of successful Bluetooth pairing events result in the LED output switching ON within a perceptibly immediate time (no noticeable delay to a human observer).
- **SC-002**: 100% of Bluetooth disconnection events result in the LED output switching OFF within a perceptibly immediate time (no noticeable delay to a human observer).
- **SC-003**: Across 10 consecutive connect/disconnect cycles in a session, the LED output state matches the actual connection state after every cycle, with zero mismatches.
- **SC-004**: An operator can, without consulting the app, correctly determine whether the car is currently paired just by looking at the LED, every time.

## Assumptions

- "Successful Bluetooth connection" refers to the same connection-established condition already recognized elsewhere in the system (e.g., existing connection event/logging behavior), not a separate or stricter definition of pairing.
- The LED is a simple, single-color, single-state indicator (no blinking, dimming, or multi-color patterns) — it only needs an ON/OFF signal.
- The physical LED, its circuit, current-limiting, and mounting outside the car are out of scope; this feature is limited to producing the correct 3.3V-level ON/OFF output signal.
- The output signal does not need to persist across power loss/reset; it is expected to re-derive from live connection state on every boot.
- No user-facing configuration (e.g., disabling the LED) is required; the indicator is always active whenever the device is powered.
