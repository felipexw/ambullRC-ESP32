# Feature Specification: Motor & Servo Actuation

**Feature Branch**: `002-motor-servo-actuation`

**Created**: 2026-07-23

**Status**: Draft

**Input**: User description: "So, now that we already receive a signal from the app and log it in the terminal, i wanna keep printing it, but i also want to move the car. In other words, every time the app sends "UP" the motor is gonna go forward, whereas every time the app sends "DOWN", it's going to invert the direction. Keep in mind that both UP and DOWN directions commands the DC motor (look at the function test that you've implemented). Also, every time the app sends "RIGHT" or "LEFT", it will only change the direction of the servo motor. In this case, every time is right, it's gonna move the servo to 180 degrees or something like that, whereas when it comes "LEFT" it's gonna go to 10 degrees. The ida here is to simulate the steering wheel of the car."

## Clarifications

### Session 2026-07-23

- Q: What should the protective pause before reversing DC motor polarity be, and how should
  SC-001 account for it? → A: Keep the existing 300ms pause (already validated by the project's
  hardware bring-up test); SC-001's budget becomes ~550ms for direction changes that reverse
  polarity, and stays 250ms for direction changes that don't.
- Q: What should happen to the standalone hardware bring-up test once real command-driven
  actuation exists? → A: Delete it entirely (code and its now-unused config constants) — the
  wiring it validated is already confirmed working, and keeping unused/dead code around
  contradicts the project's Simplicity First (YAGNI) principle.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Drive forward and backward on command (Priority: P1)

As the operator, when I send the app's "up" control the car's DC motor drives forward, and when I
send "down" the motor engages in reverse, so the car actually moves instead of only having its
intended direction printed to a log.

**Why this priority**: Actually moving the car is the entire point of this feature — deciding and
logging a direction without acting on it delivers no real driving capability.

**Independent Test**: With the app connected, send an "up" command and confirm the DC motor spins
forward; send a "down" command and confirm the motor switches to reverse; confirm the existing
serial log line for the decided direction still prints during both.

**Acceptance Scenarios**:

1. **Given** the car is connected and idle, **When** the operator sends "up", **Then** the DC
   motor drives forward and the direction is still logged to serial as before.
2. **Given** the DC motor is driving forward, **When** the operator sends "down", **Then** the
   motor switches to driving in reverse.
3. **Given** the operator stops sending a forward/backward command, **When** the decided direction
   has no drive component, **Then** the DC motor stops.

---

### User Story 2 - Steer left and right on command (Priority: P1)

As the operator, when I send "right" the steering servo turns to its full right-lock position, and
when I send "left" it turns to its full left-lock position — independent of whatever the drive
motor is doing — so I can steer the car like a steering wheel while driving forward, backward, or
standing still.

**Why this priority**: Steering is the other half of controlling the car's motion and, combined
with User Story 1, delivers the complete driving experience the app already signals for.

**Independent Test**: Send "right" and confirm the servo moves to the full-right angle regardless
of the motor's state; send "left" and confirm it moves to the full-left angle; send neither and
confirm the servo sits at its straight/neutral angle.

**Acceptance Scenarios**:

1. **Given** the car is connected, **When** the operator sends "right", **Then** the servo moves
   to its configured full-right angle.
2. **Given** the car is connected, **When** the operator sends "left", **Then** the servo moves to
   its configured full-left angle.
3. **Given** neither left nor right is the currently decided direction, **Then** the servo sits at
   its straight/neutral angle.
4. **Given** the motor is driving forward, **When** the operator sends "right", **Then** the motor
   keeps driving forward while the servo turns right — steering and driving never interfere with
   each other.

---

### User Story 3 - Fail-safe stop applies to real hardware (Priority: P1)

As the operator (and anyone nearby), when the Bluetooth connection drops, times out, or a
malformed command arrives, I need the actual DC motor to stop and the actual servo to return to
neutral — not just a log line — so the car can never keep physically moving unattended once its
motors are really being driven.

**Why this priority**: This graduates the existing print-only fail-safe into a real physical
safety behavior. Once the car can actually move, an un-actuated fail-safe would be a safety hazard
rather than just an inaccurate log line — it is as critical as driving and steering themselves.

**Independent Test**: Connect the app, send a driving or turning command, then disconnect the app
(or let the command time out) and confirm the DC motor physically stops and the servo physically
returns to neutral, matching the safe-state event already logged today.

**Acceptance Scenarios**:

1. **Given** the DC motor is driving (forward or backward), **When** the connection is lost or
   times out, **Then** the DC motor stops.
2. **Given** the servo is turned left or right, **When** the connection is lost or times out,
   **Then** the servo returns to its neutral angle.
3. **Given** a malformed command is received mid-drive, **Then** the motor and servo state remain
   unchanged, exactly matching today's "ignore it, stay safe" behavior.

---

### Edge Cases

- What happens when "up" and "down" (or "left" and "right") are received in rapid succession?
  (Resolved: FR-010 requires a 300ms pause before the motor reverses polarity; the servo has no
  such pause and switches immediately.)
- What happens when the decided direction combines both axes (e.g., forward + right) — do the
  motor and servo each apply their own component correctly and independently?
- What is the hardware's state during the brief startup window before the first command
  arrives — motor stopped and servo at neutral, matching the safe default?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST drive the DC motor forward whenever the currently decided direction
  has a forward component.
- **FR-002**: The system MUST drive the DC motor in reverse whenever the currently decided
  direction has a backward component.
- **FR-003**: The system MUST stop the DC motor whenever the currently decided direction has
  neither a forward nor a backward component.
- **FR-004**: The system MUST turn the steering servo to a configured full-right angle whenever
  the currently decided direction has a right component.
- **FR-005**: The system MUST turn the steering servo to a configured full-left angle whenever the
  currently decided direction has a left component.
- **FR-006**: The system MUST hold the steering servo at its neutral (straight) angle whenever the
  currently decided direction has neither a left nor a right component.
- **FR-007**: The system MUST apply drive (motor) and steering (servo) actuation independently
  from the same decided direction, so that changing one never unintentionally changes the other.
- **FR-008**: The system MUST continue emitting the existing direction log line unchanged
  alongside the new hardware actuation — the current logging behavior must not regress.
- **FR-009**: The system MUST stop the DC motor and return the servo to neutral immediately
  whenever it enters the existing fail-safe state (disconnect, timeout, or malformed command), so
  the fail-safe protects real hardware, not just the log.
- **FR-010**: The system MUST insert a 300ms protective pause before reversing the DC motor's
  polarity (forward↔reverse), to avoid electrical stress on the motor driver — the same value
  already validated by the project's existing hardware bring-up test.
- **FR-011**: The system MUST remove the standalone wiring bring-up test (code and its
  now-unused config constants) once real command-driven actuation is in place, since the wiring
  it validated is already confirmed working and it would otherwise contend for the same servo and
  motor pins as real actuation.

### Key Entities

- **Vehicle State** *(extends the entity from the Bluetooth Motor Control feature)*: now also
  reflects the DC motor's actual driving state (forward, reverse, or stopped) and the servo's
  actual angle, not just a printed label.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: An operator sending a forward or backward command sees the car actually move in the
  corresponding direction within 250ms, or within 550ms when the command reverses the motor's
  current polarity (250ms base response plus the mandatory 300ms protective pause), consistent
  with the project's existing responsiveness target.
- **SC-002**: An operator sending a left or right command sees the car's steering actually turn to
  the corresponding full-lock position within 250ms, matching the project's existing steering
  responsiveness target.
- **SC-003**: 100% of connection-loss, timeout, or malformed-command events result in the DC
  motor's actual rotation stopping and the servo's actual position returning to neutral, verified
  by automated tests run without physical hardware.
- **SC-004**: In 100% of tested command combinations, driving and steering can be commanded and
  observed independently of one another — changing one axis never unintentionally changes the
  other actuator's state.

## Assumptions

- **Superseded during implementation**: this assumption originally stated that "up"/"down"/
  "left"/"right" were just informal names for existing numeric commands and that no new wire
  vocabulary was needed. On-device testing showed the Android app actually sends these as literal
  per-axis word commands (`UP`/`DOWN`/`LEFT`/`RIGHT`), which the original `"<steer>,<throttle>"`-
  only parser silently rejected as malformed — no direction was ever decided, so nothing moved.
  Fixed by `DriveCommandAssembler` (`src/protocol/`), which accepts both forms: a word command
  updates only its own axis (throttle for `UP`/`DOWN`, steer for `LEFT`/`RIGHT`), preserving the
  other axis's last value, while a numeric pair still sets both axes explicitly. See
  `specs/001-bluetooth-motor-control/contracts/bluetooth-command-protocol.md` for the full wire
  format.
- Steering is discrete (full-left / straight / full-right), not proportional to the magnitude of
  the app's steering input. The exact angle values are configuration constants and may be tuned
  during implementation to match the physical servo's safe range.
- The protective pause before reversing the DC motor's polarity is fixed at 300ms (FR-010),
  matching the value already validated by the project's existing hardware bring-up test; SC-001's
  ~550ms reversal budget accounts for it directly.
- The standalone hardware bring-up test added earlier for wiring verification is deleted as part
  of this feature (FR-011): the wiring it checked is already confirmed working, and it is not
  expected to coexist with real command-driven actuation.
- This feature only adds hardware actuation on top of the existing Bluetooth protocol, direction-
  decision logic, and fail-safe timing already implemented by the Bluetooth Motor Control feature;
  no changes to those are in scope.
