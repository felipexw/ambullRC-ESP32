# Feature Specification: Bluetooth Motor Control

**Feature Branch**: `001-bluetooth-motor-control`

**Created**: 2026-07-18

**Status**: Draft

**Input**: User description: "So, this ESP32 is going to receive Bluetooth commands from the app, therefore we need to implement and deploy this feature in this ESP32"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Real-time driving over Bluetooth (Priority: P1)

As the person operating the RC car, I connect the Android app to the car over Bluetooth and send
steering and throttle commands so the car moves the way I intend, with a delay short enough that
driving feels responsive.

**Why this priority**: This is the core purpose of the project. Without it, the car cannot be
remote-controlled at all — every other capability exists to support this one.

**Independent Test**: Pair the app to the car over Bluetooth, send a sequence of steering and
throttle commands, and confirm the servo and DC motor respond correctly and promptly.

**Acceptance Scenarios**:

1. **Given** the app is connected to the car and the car is idle, **When** the operator sends a
   "drive forward" command, **Then** the DC motor drives forward at the requested speed.
2. **Given** the app is connected, **When** the operator sends a steering command, **Then** the
   servo moves to the corresponding angle.
3. **Given** the car is driving, **When** the operator sends a "stop"/neutral throttle command,
   **Then** the DC motor stops.

---

### User Story 2 - Automatic fail-safe stop (Priority: P1)

As the operator (and anyone near the car), I need the car to automatically stop the DC motor and
return the servo to neutral whenever the Bluetooth connection drops, a command doesn't arrive in
time, or a received command is malformed — so the car never keeps moving unattended or
uncontrolled.

**Why this priority**: A moving car that stops responding to its controller is a safety hazard.
This is as critical as driving itself and is a non-negotiable project requirement.

**Independent Test**: Connect the app, send a driving command, then disconnect the app (or stop
sending commands past the timeout window) and confirm the DC motor stops and the servo returns to
neutral without any further app input.

**Acceptance Scenarios**:

1. **Given** the car is driving, **When** the Bluetooth connection is lost, **Then** the DC motor
   stops and the servo returns to neutral.
2. **Given** the app is connected, **When** no valid command is received within the configured
   timeout, **Then** the car enters the same safe state as a lost connection.
3. **Given** the app is connected, **When** a malformed or unparseable command is received,
   **Then** the car ignores it, remains safe, and does not crash or hang.
4. **Given** the car is in a safe state after a disconnect or timeout, **When** a new valid
   command is received, **Then** the car resumes normal control without requiring a separate
   "re-arm" action.

---

### User Story 3 - Reject out-of-range commands (Priority: P2)

As the operator, I want steering or throttle values outside the car's safe operating range to be
rejected outright rather than applied, so a corrupted or badly formed command can never push the
motor to an unsafe extreme.

**Why this priority**: This hardens the system against subtly bad input (as opposed to a full
disconnect, covered by User Story 2) but delivers value even on its own.

**Independent Test**: With the app connected, send a steering or throttle value outside the
configured safe range and confirm the command is rejected and the car's prior state is preserved.

**Acceptance Scenarios**:

1. **Given** the car is connected, **When** a steering command exceeds the configured safe angle
   range, **Then** the command is rejected and the servo stays at its last valid position.
2. **Given** the car is connected, **When** a throttle command exceeds the configured safe speed
   range, **Then** the command is rejected and the DC motor stays at its last valid speed.

---

### Edge Cases

- What happens when two commands arrive faster than the control loop can process them — is the
  newest command applied, or are they queued?
- How does the system behave if a command arrives before startup/initialization has completed?
- What happens if the app sends a very large or garbled payload (e.g., a truncated or corrupted
  transmission) — it must be safely discarded, not partially applied.
- What happens if the Bluetooth connection flaps (repeated connect/disconnect in quick
  succession) — the car must stay in a safe state throughout, never toggling into an active drive
  state on a partial/unstable connection.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST allow the Android app to establish a Bluetooth connection to the
  car.
- **FR-002**: The system MUST receive discrete drive commands over the connection, each
  specifying a steering angle and/or throttle level.
- **FR-003**: The system MUST convert valid steering commands into servo position changes,
  clamped within a configured safe angle range.
- **FR-004**: The system MUST convert valid throttle commands into DC motor speed and direction
  changes, clamped within a configured safe range.
- **FR-005**: The system MUST reject any command whose requested steering or throttle value falls
  outside the configured safe range, rather than silently clamping it to the boundary.
- **FR-006**: The system MUST discard malformed or unparseable command data without applying it
  to the motors and without crashing or hanging the device.
- **FR-007**: The system MUST detect when the Bluetooth connection to the app is lost and, upon
  detection, stop the DC motor and return the servo to a defined neutral position.
- **FR-008**: The system MUST detect when no valid command has been received within a configured
  timeout interval and enter the same safe state as a lost connection.
- **FR-009**: The system MUST resume normal control automatically as soon as a new valid command
  is received after a safe-state event, without requiring a separate re-arm step.
- **FR-010**: The system MUST document the Bluetooth command protocol (message formats and
  versioning) in the repository so it can evolve without silently breaking compatibility.
- **FR-011**: The system MUST accept drive commands from any Bluetooth device that successfully
  connects; no pairing/authorization check beyond a successful connection is required. This is an
  accepted tradeoff for a private hobby project (see Assumptions).

### Key Entities

- **Drive Command**: A single instruction sent from the app representing the desired steering
  angle and/or throttle level at a point in time.
- **Vehicle State**: The car's current operating condition — driving or safe/stopped — along with
  its current servo angle and motor speed/direction.
- **Bluetooth Session**: An active connection between the app and the car; tracks
  connected/disconnected status and the time of the last valid command received, used to detect
  timeouts.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: An operator can connect the app to the car and issue a working steering or throttle
  command within 15 seconds of powering on the car.
- **SC-002**: 95% of driving commands produce a corresponding motor or servo response within
  250ms of being sent from the app.
- **SC-003**: 100% of connection-loss events result in the DC motor stopping and the servo
  returning to neutral, verified by automated tests run without physical hardware.
- **SC-004**: 100% of malformed or out-of-range commands are rejected without the car entering an
  unintended or unsafe motor state, verified by automated tests.

## Assumptions

- The car accepts commands from any Bluetooth device that connects, with no pairing/authorization
  check beyond a successful connection. This is a deliberate simplicity tradeoff for a private
  hobby project used in controlled settings (e.g., home/yard), not a public or shared vehicle;
  revisit if the deployment context changes.
- The app and car communicate one-to-one; supporting multiple simultaneous app connections to one
  car is out of scope for this feature.
- A command-to-motion latency target of 250ms is a reasonable default for a responsive driving
  feel; the exact figure may be tuned during implementation.
- After a fail-safe stop (disconnect, timeout, or malformed command), the car automatically
  resumes normal control as soon as a new valid command arrives — no explicit "re-arm" action is
  required from the operator (kept simple per the project's YAGNI principle).
- The specific safe ranges for steering angle and motor speed depend on the physical servo/motor
  hardware installed; exact numeric bounds are a deployment-time configuration detail, not a fixed
  value in this spec.
- The Bluetooth command protocol's wire format (encoding, field layout) is a technical design
  decision to be made during planning, not specified here.
