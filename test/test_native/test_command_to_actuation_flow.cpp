#include <unity.h>

#include "config.h"
#include "control/direction_control.h"
#include "fakes/fake_motor_driver.h"
#include "fakes/fake_steering_servo.h"
#include "fakes/fake_transport.h"
#include "fakes/recording_vehicle_output.h"
#include "hardware/motor_servo_vehicle_output.h"
#include "protocol/command_parser.h"
#include "protocol/drive_command_assembler.h"

// Integration tests: FakeTransport -> parseLine -> DirectionControl -> both
// RecordingVehicleOutput (serial log, unchanged from 001) and
// MotorServoVehicleOutput (real actuation, against FakeMotorDriver/
// FakeSteeringServo).
// US1: forward/backward commands drive the motor, respecting the reversal
//      pause.
// US2: left/right (and combined) commands steer the servo, independent of
//      motor state.
// US3: disconnect and malformed commands stop the motor and center the
//      servo immediately, exactly like the existing serial fail-safe.

namespace {

void pumpCommand(FakeTransport& transport, DirectionControl& control, RecordingVehicleOutput& log,
                  MotorServoVehicleOutput& hardware, unsigned long nowMs) {
  std::string line;
  if (!transport.readLine(line)) return;
  DriveCommand cmd;
  if (parseLine(line, cmd) != ParseResult::Ok) return;
  Direction direction = control.onCommand(cmd, nowMs);
  log.emit(direction);
  hardware.emit(direction);
}

void tickFailSafe(FakeTransport& transport, DirectionControl& control, RecordingVehicleOutput& log,
                   MotorServoVehicleOutput& hardware, unsigned long nowMs) {
  Direction direction;
  if (control.onTick(transport.connected(), nowMs, direction)) {
    log.emit(direction);
    hardware.emit(direction);
  }
}

// Mirrors main.cpp's real wiring: the app sends per-axis word commands
// ("UP"/"DOWN"/"LEFT"/"RIGHT"), assembled via DriveCommandAssembler.
void pumpWordCommand(FakeTransport& transport, DriveCommandAssembler& assembler,
                      DirectionControl& control, RecordingVehicleOutput& log,
                      MotorServoVehicleOutput& hardware, unsigned long nowMs) {
  std::string line;
  if (!transport.readLine(line)) return;
  DriveCommand cmd;
  if (assembler.apply(line, cmd) != ParseResult::Ok) return;
  Direction direction = control.onCommand(cmd, nowMs);
  log.emit(direction);
  hardware.emit(direction);
}

}  // namespace

void test_actuation_flow_forward_then_reverse_respects_pause(void) {
  FakeTransport transport;
  DirectionControl control;
  RecordingVehicleOutput log;
  FakeMotorDriver motor;
  FakeSteeringServo servo;
  MotorServoVehicleOutput hardware(motor, servo);

  transport.enqueueLine("0,100");  // FORWARD
  pumpCommand(transport, control, log, hardware, 1000);
  hardware.tick(1000);
  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Forward), static_cast<int>(motor.last()));

  transport.enqueueLine("0,-100");  // BACKWARD: a true reversal
  pumpCommand(transport, control, log, hardware, 1010);
  hardware.tick(1010);
  // Reversal begins: stopped immediately, not yet reversed.
  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Stop), static_cast<int>(motor.last()));

  hardware.tick(1010 + config::kMotorReversePauseMs);
  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Reverse), static_cast<int>(motor.last()));

  TEST_ASSERT_EQUAL(2, log.emitted().size());
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Forward), static_cast<int>(log.emitted()[0]));
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Backward), static_cast<int>(log.emitted()[1]));
}

void test_actuation_flow_steering_independent_of_motor(void) {
  FakeTransport transport;
  DirectionControl control;
  RecordingVehicleOutput log;
  FakeMotorDriver motor;
  FakeSteeringServo servo;
  MotorServoVehicleOutput hardware(motor, servo);

  transport.enqueueLine("-50,0");  // LEFT
  pumpCommand(transport, control, log, hardware, 1000);
  hardware.tick(1000);
  TEST_ASSERT_EQUAL(config::kServoLeftAngleDeg, servo.last());

  transport.enqueueLine("50,100");  // FORWARD_RIGHT
  pumpCommand(transport, control, log, hardware, 1010);
  hardware.tick(1010);
  TEST_ASSERT_EQUAL(config::kServoRightAngleDeg, servo.last());
  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Forward), static_cast<int>(motor.last()));

  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Left), static_cast<int>(log.emitted()[0]));
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::ForwardRight), static_cast<int>(log.emitted()[1]));
}

void test_actuation_flow_disconnect_stops_motor_and_centers_servo_immediately(void) {
  FakeTransport transport;
  DirectionControl control;
  RecordingVehicleOutput log;
  FakeMotorDriver motor;
  FakeSteeringServo servo;
  MotorServoVehicleOutput hardware(motor, servo);

  transport.enqueueLine("50,100");  // FORWARD_RIGHT
  pumpCommand(transport, control, log, hardware, 1000);
  hardware.tick(1000);

  transport.setConnected(false);
  tickFailSafe(transport, control, log, hardware, 1010);
  hardware.tick(1010);

  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Stop), static_cast<int>(motor.last()));
  TEST_ASSERT_EQUAL(config::kServoNeutralAngleDeg, servo.last());
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Stop), static_cast<int>(log.last()));
}

void test_actuation_flow_disconnect_mid_pause_stays_stopped(void) {
  FakeTransport transport;
  DirectionControl control;
  RecordingVehicleOutput log;
  FakeMotorDriver motor;
  FakeSteeringServo servo;
  MotorServoVehicleOutput hardware(motor, servo);

  transport.enqueueLine("0,100");  // FORWARD
  pumpCommand(transport, control, log, hardware, 1000);
  hardware.tick(1000);

  transport.enqueueLine("0,-100");  // BACKWARD: begins the reversal pause
  pumpCommand(transport, control, log, hardware, 1010);
  hardware.tick(1010);
  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Stop), static_cast<int>(motor.last()));

  // Disconnect mid-pause.
  transport.setConnected(false);
  tickFailSafe(transport, control, log, hardware, 1020);
  hardware.tick(1020);
  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Stop), static_cast<int>(motor.last()));

  // Even once the original pause window would have elapsed, the motor must
  // not resurrect the pre-disconnect reverse polarity.
  hardware.tick(1010 + config::kMotorReversePauseMs);
  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Stop), static_cast<int>(motor.last()));
}

void test_actuation_flow_malformed_line_does_not_change_hardware_state(void) {
  FakeTransport transport;
  DirectionControl control;
  RecordingVehicleOutput log;
  FakeMotorDriver motor;
  FakeSteeringServo servo;
  MotorServoVehicleOutput hardware(motor, servo);

  transport.enqueueLine("0,100");  // FORWARD
  pumpCommand(transport, control, log, hardware, 1000);
  hardware.tick(1000);

  transport.enqueueLine("garbage");
  pumpCommand(transport, control, log, hardware, 1010);
  hardware.tick(1010);

  TEST_ASSERT_EQUAL(1, log.emitted().size());
  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Forward), static_cast<int>(motor.last()));
}

// Reproduces the real-world bug report: the app sends "RIGHT" once and the
// servo (continuous-rotation, not positional) spins forever because nothing
// ever tells it to stop — the connection stays active and no further
// command ever arrives. It must self-stop after config::kServoTurnPulseMs.
void test_actuation_flow_turn_auto_stops_without_further_commands(void) {
  FakeTransport transport;
  DriveCommandAssembler assembler;
  DirectionControl control;
  RecordingVehicleOutput log;
  FakeMotorDriver motor;
  FakeSteeringServo servo;
  MotorServoVehicleOutput hardware(motor, servo);

  transport.enqueueLine("RIGHT");
  pumpWordCommand(transport, assembler, control, log, hardware, 1000);
  hardware.tick(1000);
  TEST_ASSERT_EQUAL(config::kServoRightAngleDeg, servo.last());

  // No release/center command ever comes in (matching the real app's
  // sticky per-axis word protocol) — only time passing via tick().
  hardware.tick(1000 + config::kServoTurnPulseMs);
  TEST_ASSERT_EQUAL(config::kServoNeutralAngleDeg, servo.last());
}

// Reproduces the real-world bug: the app sends "UP" then "RIGHT" (two
// separate per-axis word commands, not a single "<steer>,<throttle>" pair).
// The motor must keep driving forward while the servo turns right — neither
// command may reset the other axis.
void test_actuation_flow_word_commands_up_then_right_stay_independent(void) {
  FakeTransport transport;
  DriveCommandAssembler assembler;
  DirectionControl control;
  RecordingVehicleOutput log;
  FakeMotorDriver motor;
  FakeSteeringServo servo;
  MotorServoVehicleOutput hardware(motor, servo);

  transport.enqueueLine("UP");
  pumpWordCommand(transport, assembler, control, log, hardware, 1000);
  hardware.tick(1000);
  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Forward), static_cast<int>(motor.last()));
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Forward), static_cast<int>(log.last()));

  transport.enqueueLine("RIGHT");
  pumpWordCommand(transport, assembler, control, log, hardware, 1010);
  hardware.tick(1010);

  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Forward), static_cast<int>(motor.last()));
  TEST_ASSERT_EQUAL(config::kServoRightAngleDeg, servo.last());
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::ForwardRight), static_cast<int>(log.last()));
}
