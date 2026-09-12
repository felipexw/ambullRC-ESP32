# Feature Specification: Four-Light Auxiliary Control

**Feature Branch**: `006-four-led-lights-control`

**Created**: 2026-09-11

**Status**: Draft

**Input**: User description: "add a new feature to switch on/off 4 leds, each one in one GPIO of the esp32. just as the current behaviour already implemented in the components of the circuit, the command will come from the app. also, the esp32 should send to the app via bluetooth which state the lights are currently (e.g. ON/OFF)."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Turn an individual light on or off from the app (Priority: P1)

As the operator of the RC car, I want to switch any one of the four auxiliary lights on or off from the app, so that I can control the car's lighting (e.g. headlights, brake light, indicators) independently while driving.

**Why this priority**: This is the core value of the feature — without individual on/off control, there is no lighting feature at all.

**Independent Test**: Can be fully tested by sending an on command for one light and confirming only that light's output switches ON while the other three remain unchanged, then sending an off command and confirming it switches OFF.

**Acceptance Scenarios**:

1. **Given** a light is currently OFF, **When** the operator sends an ON command for that light, **Then** that light's output switches ON and the other three lights are unaffected.
2. **Given** a light is currently ON, **When** the operator sends an OFF command for that light, **Then** that light's output switches OFF and the other three lights are unaffected.
3. **Given** a light is already ON, **When** the operator sends an ON command for that same light again, **Then** the light remains ON (no flicker or toggle side effect).

---

### User Story 2 - App is told the current state after every change (Priority: P1)

As the operator, whenever I switch a light on or off, I want the app to receive confirmation of that light's actual current state, so that the app's display always matches what the car is really doing rather than just assuming the command worked.

**Why this priority**: A control without feedback is unreliable — the operator needs to trust that what the app shows matches reality, especially since Bluetooth commands can be dropped or arrive out of order.

**Independent Test**: Can be fully tested by sending a light command and observing that a state report for that light (ON/OFF) is sent back to the app reflecting the light's new actual state.

**Acceptance Scenarios**:

1. **Given** the app sends a command switching a light ON, **When** the light's output switches ON, **Then** the ESP32 sends the app a report that the light is ON.
2. **Given** the app sends a command switching a light OFF, **When** the light's output switches OFF, **Then** the ESP32 sends the app a report that the light is OFF.

---

### User Story 3 - App syncs all light states on (re)connection (Priority: P2)

As the operator, when I open the app and connect (or reconnect after a dropped connection), I want the app to immediately show the current on/off state of all four lights, so that the app's display is accurate from the start instead of defaulting to a guess.

**Why this priority**: Without this, a freshly connected or reconnected app has no way to know the lights' real state until the operator happens to toggle each one, which is confusing and error-prone.

**Independent Test**: Can be fully tested by setting the lights to a known mixed state, disconnecting and reconnecting the app, and confirming the app receives a state report for all four lights matching that known state without any light being toggled.

**Acceptance Scenarios**:

1. **Given** the lights are in a mixed ON/OFF state, **When** the app establishes a new Bluetooth connection, **Then** the ESP32 sends the app the current state of all four lights.
2. **Given** the app reconnects after a dropped connection, **When** the connection is re-established, **Then** the ESP32 sends the app the current state of all four lights, unchanged from before the drop.

---

### Edge Cases

- What happens when a command targets a light number that doesn't exist (only four are defined)? The command MUST be rejected and MUST NOT change the state of any light.
- What happens when a malformed light command is received? It MUST be rejected without affecting any light's current state, consistent with how other malformed commands are handled.
- What happens when the Bluetooth connection drops while a light is ON? The light MUST hold its last commanded state (lights are not part of the vehicle's motion fail-safe) until the app reconnects and/or sends a new command.
- What happens when the device powers on? All four lights MUST start OFF until the app sends a command, since no prior state exists to restore.
- What happens if two commands for the same light arrive in rapid succession? The light MUST end up reflecting the most recently received valid command, and a state report MUST be sent for the resulting state.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST provide four independently addressable light outputs.
- **FR-002**: System MUST allow the app to switch any single light ON or OFF via a command that identifies which of the four lights it targets.
- **FR-003**: Switching one light MUST NOT change the state of any of the other three lights.
- **FR-004**: System MUST send the app a report of a light's current state (ON/OFF) whenever that light's state changes as a result of a command.
- **FR-005**: System MUST send the app a report of the current state of all four lights whenever a new Bluetooth connection is established (including reconnection).
- **FR-006**: System MUST default all four lights to OFF at power-on/boot, before any command has been received.
- **FR-007**: System MUST reject commands that target a light number outside the four defined lights, without changing any light's state.
- **FR-008**: System MUST reject malformed light commands without changing any light's state.
- **FR-009**: Light state MUST persist through a Bluetooth disconnect (i.e., not forced OFF or reset by disconnection alone); it changes only via a new valid command or a power cycle.

### Key Entities

- **Light Output**: One of four independently controlled outputs, each with a state of ON or OFF and an identifier (1–4) used to target it in commands and in state reports.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: An operator can switch any one of the four lights ON or OFF and see the physical effect within a perceptibly immediate time (no noticeable delay to a human observer), with 100% accuracy on which light responds.
- **SC-002**: Across 20 consecutive on/off commands spread across all four lights, the app's displayed state matches the light's actual state after every command, with zero mismatches.
- **SC-003**: After connecting or reconnecting, the app displays the correct current state of all four lights within a perceptibly immediate time, without requiring the operator to toggle any light first.
- **SC-004**: Commands targeting an undefined light number or that are malformed never change the state of any of the four real lights, verified across repeated invalid-command attempts.

## Assumptions

- The four lights are simple single-color, single-state (ON/OFF only) outputs — no dimming, blinking, or color patterns are in scope.
- "State report" means the ESP32 proactively sends light state information to the app over the existing Bluetooth link; it is not the app polling/pulling state on demand.
- Lights are auxiliary/cosmetic (e.g., headlights, brake light, indicators) and are not part of the vehicle's motion-safety fail-safe behavior (Principle V applies to the DC motor and steering servo only); therefore lights are not forced OFF on disconnect or timeout.
- The physical GPIO wiring, current-limiting resistors, and placement of the four LEDs are hardware-integration details to be worked out during planning/implementation, not part of this specification.
- No user authentication/permission distinctions are needed — any app connected to the ESP32 may control all four lights, consistent with the rest of the existing control surface.
