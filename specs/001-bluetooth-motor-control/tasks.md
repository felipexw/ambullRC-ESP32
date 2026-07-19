---
description: "Task list for Bluetooth Motor Control (direction-print slice)"
---

# Tasks: Bluetooth Motor Control (direction-print slice)

**Input**: Design documents from `/specs/001-bluetooth-motor-control/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/bluetooth-command-protocol.md, quickstart.md

**Tests**: Included and REQUIRED — Constitution Principle II (Test-First) is non-negotiable for
this project: unit tests for parsing/control logic and an integration test for the command-in →
output-out path are mandatory, not optional.

**Scope reminder** (from plan.md): this slice parses Bluetooth commands and prints the resulting
`Direction` to serial. **No servo or DC-motor output is implemented here.**

**Organization**: Tasks are grouped by user story (spec.md) to enable independent implementation
and testing of each story, within this slice's print-only scope.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)

## Path Conventions

Single embedded project per plan.md: `src/` split into `transport/`, `protocol/`, `control/`,
`hardware/`; tests in `test/test_native/`.

---

## Phase 1: Setup

**Purpose**: Project scaffolding for the layered structure

- [X] T001 Create the layered directory structure: `src/transport/`, `src/protocol/`,
      `src/control/`, `src/hardware/`, `test/test_native/fakes/` (per plan.md Project Structure)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Shared types, interfaces, and test doubles that every user story depends on

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T002 [P] Define centralized config constants (`kCommandTimeoutMs`, `kSteerRange`,
      `kThrottleRange`) in `src/config.h` — no magic numbers scattered through the code, per
      AGENTS.md conventions
- [X] T003 [P] Define the `Direction` enum (`STOP`, `FORWARD`, `BACKWARD`, `LEFT`, `RIGHT`,
      `FORWARD_LEFT`, `FORWARD_RIGHT`, `BACKWARD_LEFT`, `BACKWARD_RIGHT`) in
      `src/control/direction.h` per data-model.md
- [X] T004 [P] Define the `DriveCommand` struct (`int steer`, `int throttle`) in
      `src/protocol/command_parser.h` per data-model.md
- [X] T005 [P] Define the `ITransport` interface (`bool connected()`, `bool readLine(String&)`) in
      `src/transport/i_transport.h` per plan.md
- [X] T006 [P] Define the `IVehicleOutput` interface (`void emit(Direction)`) in
      `src/hardware/i_vehicle_output.h` per plan.md
- [X] T007 [P] Implement `FakeTransport` test double (queue lines, toggle connected state) in
      `test/test_native/fakes/fake_transport.h`
- [X] T008 [P] Implement `RecordingVehicleOutput` test double (records every emitted `Direction`)
      in `test/test_native/fakes/recording_vehicle_output.h`

**Checkpoint**: Foundation ready — user story implementation can now begin

---

## Phase 3: User Story 1 - Real-time driving over Bluetooth (Priority: P1) 🎯 MVP

**Goal**: A well-formed `<steer>,<throttle>` command received over the connection produces the
correct `Direction`, printed to serial (no motor output).

**Independent Test**: Per quickstart.md's Direction table, feed valid command lines through
`FakeTransport` → `CommandParser` → `DirectionControl` → `RecordingVehicleOutput` and assert each
line's expected `Direction` is recorded.

### Tests for User Story 1 ⚠️

> Write these tests FIRST, ensure they FAIL before implementation

- [X] T009 [P] [US1] Unit tests: well-formed `<steer>,<throttle>` lines parse into the correct
      `DriveCommand` in `test/test_native/test_command_parser.cpp`
- [X] T010 [P] [US1] Unit tests: `DriveCommand` → `Direction` mapping for all 9 cases in the
      data-model.md table, in `test/test_native/test_direction_control.cpp`
- [X] T011 [US1] Integration test: `FakeTransport` feeds a sequence of valid command lines through
      the full chain to `RecordingVehicleOutput`, asserting each expected `Direction`, in
      `test/test_native/test_command_to_direction_flow.cpp` (depends on T007, T008)

### Implementation for User Story 1

- [X] T012 [US1] Implement `parseLine()` happy path in `src/protocol/command_parser.cpp`: parse
      `<int>,<int>` into a `DriveCommand`
- [X] T013 [US1] Implement `decide()` in `src/control/direction_control.cpp`: map a `DriveCommand`
      to a `Direction` per the data-model.md table
- [X] T014 [US1] Implement `SerialDirectionOutput` (`IVehicleOutput`) in
      `src/hardware/serial_direction_output.h`: `Serial.println()` the direction — no motor I/O
- [X] T015 [US1] Implement `BluetoothTransport` (`ITransport`) in
      `src/transport/bluetooth_transport.h` using the ESP32 Arduino core's `BluetoothSerial`
- [X] T016 [US1] Wire Transport → Protocol → Control → Hardware in `src/main.cpp` for the
      `esp32dev` build: read a line, parse it, decide the direction, emit it

**Checkpoint**: User Story 1 is fully functional and independently testable (native tests +
quickstart.md on-device steps for valid commands)

---

## Phase 4: User Story 2 - Automatic fail-safe stop (Priority: P1)

**Goal**: Disconnect or command timeout prints `STOP` exactly once and holds it; a malformed line
is discarded without crashing; a new valid command after a safe-state resumes normal output
automatically.

**Independent Test**: Drive `FakeTransport` through a disconnect and, separately, a
timeout (via a fake clock), asserting `RecordingVehicleOutput` sees exactly one `STOP` per
transition and resumes on the next valid command; feed a malformed line and assert no crash and
no spurious output.

### Tests for User Story 2 ⚠️

> Write these tests FIRST, ensure they FAIL before implementation

- [X] T017 [P] [US2] Unit tests: malformed lines (wrong format, non-numeric, missing field) are
      rejected — no `DriveCommand` produced, in `test/test_native/test_command_parser.cpp`
- [X] T018 [P] [US2] Unit tests: `ConnectionState` transitions `ACTIVE → SAFE` on disconnect and
      on timeout, `STOP` is decided exactly once per transition, and a new valid command resumes
      `ACTIVE` automatically, in `test/test_native/test_direction_control.cpp`
- [X] T019 [US2] Integration test: simulate a disconnect and, separately, a timeout via
      `FakeTransport`/fake clock, and a malformed line, asserting `RecordingVehicleOutput` records
      exactly one `STOP` per transition, nothing for the malformed line, and resumes correctly, in
      `test/test_native/test_command_to_direction_flow.cpp`

### Implementation for User Story 2

- [X] T020 [US2] Extend `command_parser.cpp` to detect and reject malformed lines (return a
      failure result, no `DriveCommand`) without crashing or hanging — already covered by T012's
      strict format parser (verified by T017's new tests, no additional production code needed)
- [X] T021 [US2] Add `ConnectionState` (connected flag, `lastCommandAtMs`) and safe-state
      transition logic to `src/control/direction_control.{h,cpp}`, using `kCommandTimeoutMs` from
      `src/config.h`, emitting `STOP` exactly once per transition into the safe state
- [X] T022 [US2] Update the `src/main.cpp` loop to feed transport connected-state and elapsed time
      into `direction_control` each iteration so disconnects and timeouts are detected

**Checkpoint**: User Stories 1 and 2 both work independently

---

## Phase 5: User Story 3 - Reject out-of-range commands (Priority: P2)

**Goal**: A syntactically valid command whose `steer` or `throttle` falls outside `[-100, 100]` is
rejected outright (not clamped), and the previously decided `Direction` is preserved.

**Independent Test**: Send one valid in-range command, then one out-of-range command, and assert
`RecordingVehicleOutput`'s last recorded `Direction` still matches the valid command — the
out-of-range command produced no new output.

### Tests for User Story 3 ⚠️

> Write these tests FIRST, ensure they FAIL before implementation

- [X] T023 [P] [US3] Unit tests: boundary values `-100`/`100` are accepted, `-101`/`101` are
      rejected for both `steer` and `throttle`, in `test/test_native/test_command_parser.cpp`
- [X] T024 [US3] Integration test: send a valid command then an out-of-range command, asserting
      the recorded `Direction` is unchanged by the rejected command, in
      `test/test_native/test_command_to_direction_flow.cpp`

### Implementation for User Story 3

- [X] T025 [US3] Extend `command_parser.cpp` to validate `steer`/`throttle` against
      `kSteerRange`/`kThrottleRange` from `src/config.h` and reject (not clamp) out-of-range
      values

**Checkpoint**: All three user stories are independently functional per quickstart.md

---

## Phase 6: Polish & Cross-Cutting Concerns

- [X] T026 [P] Reconcile `contracts/bluetooth-command-protocol.md` with any wire-format edge case
      discovered during implementation (e.g., exact malformed-input examples)
- [X] T027 Run `pio test -e native` and confirm the full suite (T009–T025 plus the existing
      `test/test_native/test_dummy.cpp`) passes — 34/34 passed
- [ ] T028 Run the on-device validation steps in `quickstart.md` against a flashed `esp32dev`
      build and confirm every row of the Direction table — **NOT DONE**: no ESP32 is currently
      connected to this machine (`pio device list` shows no `usbserial` port), and this step
      needs a phone with an SPP terminal app to actually send commands. Requires manual follow-up.

---

## Phase 7: Connection lifecycle logging (post-slice addition)

**Goal**: Log every successful Bluetooth connection and every disconnection to serial, exactly
once per transition — independent of drive-command parsing.

- [X] T029 [P] Unit tests: `ConnectionMonitor::onTick()` reports `Connected`/`Disconnected`
      exactly once per transition of the transport's `connected()` flag, and `None` while
      unchanged, in `test/test_native/test_connection_monitor.cpp`
- [X] T030 [P] Implement `ConnectionEvent` enum in `src/control/connection_event.h`, and
      `ConnectionMonitor` (pure edge-detection) in `src/control/connection_monitor.{h,cpp}`
- [X] T031 [P] Implement `IConnectionOutput` interface in `src/hardware/i_connection_output.h`
      and `SerialConnectionOutput` (`Serial.println`) in `src/hardware/serial_connection_output.h`
- [X] T032 Integration test: `FakeTransport` connect/disconnect/reconnect sequence produces the
      expected `ConnectionEvent` log via `RecordingConnectionOutput`, in
      `test/test_native/test_connection_logging_flow.cpp`
- [X] T033 Wire `ConnectionMonitor` + `SerialConnectionOutput` into `src/main.cpp`'s `loop()`,
      polled once per iteration alongside the existing command/safe-state handling
- [X] T034 Update `contracts/bluetooth-command-protocol.md` and `quickstart.md` with the
      connection-logging behavior and on-device validation steps

---

## Phase 8: Connection identification (post-slice addition)

**Goal**: Tag each connection lifecycle log with the connecting device's Bluetooth MAC address
(no pairing/name resolution is performed, so the address is the identifier).

- [X] T035 [P] Extend `ITransport` with `deviceId()` in `src/transport/i_transport.h`; add
      `setDeviceId()`/`deviceId()` to `FakeTransport` in `test/test_native/fakes/fake_transport.h`
- [X] T036 [P] Extend `IConnectionOutput::emit()` to take a `deviceId` string in
      `src/hardware/i_connection_output.h`; update `SerialConnectionOutput` to print it
      (`src/hardware/serial_connection_output.h`) and `RecordingConnectionOutput` to record it
      (`test/test_native/fakes/recording_connection_output.h`)
- [X] T037 [US] Unit/integration tests: device id is captured on connect and retained through the
      following disconnect log, in `test/test_native/test_connection_logging_flow.cpp`
- [X] T038 Implement MAC-address capture in `BluetoothTransport` via `register_callback()` on
      `ESP_SPP_SRV_OPEN_EVT` (`param->srv_open.rem_bda`), formatted as `AA:BB:CC:DD:EE:FF`, in
      `src/transport/bluetooth_transport.h`
- [X] T039 Wire `transport.deviceId()` into the `connectionOutput.emit()` call in `src/main.cpp`
- [X] T040 Update `contracts/bluetooth-command-protocol.md` and `quickstart.md` with the
      device-id logging format

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately
- **Foundational (Phase 2)**: Depends on Setup — BLOCKS all user stories
- **User Story 1 (Phase 3)**: Depends on Foundational only
- **User Story 2 (Phase 4)**: Depends on Foundational; extends the same `command_parser`/
  `direction_control` files US1 creates, so implement after US1 for a clean diff (both are P1)
- **User Story 3 (Phase 5)**: Depends on Foundational; extends `command_parser` again — implement
  after US2
- **Polish (Phase 6)**: Depends on all desired user stories being complete

### Within Each User Story

- Tests MUST be written and FAIL before implementation (Constitution Principle II)
- Shared types/interfaces (Foundational) before parser/control logic
- Parser and control logic before hardware/transport wiring
- `main.cpp` wiring last, once the pieces it wires together exist

### Parallel Opportunities

- All Foundational tasks marked [P] (T002–T008) can run in parallel — different files, no
  cross-dependencies
- Within each story, unit test tasks marked [P] can run in parallel with each other (different
  test files); the integration test depends on the fakes and typically comes last among tests
- T026 (docs) can run in parallel with T027/T028 (validation)

---

## Parallel Example: User Story 1

```bash
# Unit tests for US1 together:
Task: "Unit tests: well-formed lines parse into DriveCommand in test/test_native/test_command_parser.cpp"
Task: "Unit tests: DriveCommand -> Direction mapping in test/test_native/test_direction_control.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (blocks everything)
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: `pio test -e native`, then the valid-command rows of quickstart.md
5. This is the smallest slice that proves commands-in → direction-out works

### Incremental Delivery

1. Setup + Foundational → foundation ready
2. User Story 1 → validate independently (MVP: driving commands print the right direction)
3. User Story 2 → validate independently (disconnect/timeout/malformed → safe `STOP`, then resume)
4. User Story 3 → validate independently (out-of-range rejected, prior direction preserved)
5. Polish → full native suite + full quickstart.md pass

---

## Notes

- No servo/DC-motor code is introduced anywhere in this task list — that is a future feature built
  on top of the same `IVehicleOutput` interface (per plan.md Summary)
- Commit after each task or logical group, per AGENTS.md conventions
- Verify each test fails before implementing the corresponding production code
