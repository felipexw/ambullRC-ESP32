#include <unity.h>

#include "config.h"
#include "control/direction.h"
#include "hardware/motor_servo_vehicle_output.h"
#include "fakes/fake_motor_driver.h"
#include "fakes/fake_steering_servo.h"

// User Story 1: FORWARD/BACKWARD/STOP apply immediately when starting from
// Stopped (no prior polarity to protect against reversing out of).

void test_output_forward_drives_forward_immediately(void) {
  FakeMotorDriver motor;
  FakeSteeringServo servo;
  MotorServoVehicleOutput output(motor, servo);

  output.emit(Direction::Forward);
  output.tick(0);

  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Forward),
                     static_cast<int>(motor.last()));
}

void test_output_backward_drives_reverse_immediately(void) {
  FakeMotorDriver motor;
  FakeSteeringServo servo;
  MotorServoVehicleOutput output(motor, servo);

  output.emit(Direction::Backward);
  output.tick(0);

  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Reverse),
                     static_cast<int>(motor.last()));
}

void test_output_stop_stops_immediately(void) {
  FakeMotorDriver motor;
  FakeSteeringServo servo;
  MotorServoVehicleOutput output(motor, servo);

  output.emit(Direction::Forward);
  output.tick(0);

  output.emit(Direction::Stop);
  output.tick(0);

  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Stop),
                     static_cast<int>(motor.last()));
}

// User Story 1: a true reversal (Forward<->Backward) must stop immediately
// and hold off engaging the new polarity until the protective pause elapses.

void test_output_reversal_stops_immediately_then_waits_before_reversing(void) {
  FakeMotorDriver motor;
  FakeSteeringServo servo;
  MotorServoVehicleOutput output(motor, servo);

  output.emit(Direction::Forward);
  output.tick(0);
  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Forward), static_cast<int>(motor.last()));

  output.emit(Direction::Backward);
  output.tick(0);
  // Reversal begins: motor must be stopped immediately, not yet reversed.
  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Stop), static_cast<int>(motor.last()));

  // Just before the pause elapses: still must not have engaged Reverse.
  output.tick(config::kMotorReversePauseMs - 1);
  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Stop), static_cast<int>(motor.last()));

  // Pause elapsed: now it must engage the new polarity.
  output.tick(config::kMotorReversePauseMs);
  TEST_ASSERT_EQUAL(static_cast<int>(FakeMotorDriver::Call::Reverse), static_cast<int>(motor.last()));
}

// Reproduces the real hardware bug: the steering servo is continuous-
// rotation, not positional — it has no way to hold a fixed angle, only a
// speed/direction, so a sustained non-neutral pulse spins it forever. A
// turn direction must therefore auto-stop after a bounded pulse
// (config::kServoTurnPulseMs) even if no new Direction is ever emitted.

void test_output_right_turn_auto_stops_after_pulse_duration_with_no_new_command(void) {
  FakeMotorDriver motor;
  FakeSteeringServo servo;
  MotorServoVehicleOutput output(motor, servo);

  output.emit(Direction::Right);
  output.tick(0);
  TEST_ASSERT_EQUAL(config::kServoRightAngleDeg, servo.last());

  // Just before the pulse elapses, it must still be mid-turn.
  output.tick(config::kServoTurnPulseMs - 1);
  TEST_ASSERT_EQUAL(config::kServoRightAngleDeg, servo.last());

  // Pulse elapsed, still no new command: must auto-stop (this is the fix —
  // previously nothing ever re-wrote the stop pulse here, so it spun
  // forever).
  output.tick(config::kServoTurnPulseMs);
  TEST_ASSERT_EQUAL(config::kServoNeutralAngleDeg, servo.last());
}

void test_output_left_turn_auto_stops_after_pulse_duration_with_no_new_command(void) {
  FakeMotorDriver motor;
  FakeSteeringServo servo;
  MotorServoVehicleOutput output(motor, servo);

  output.emit(Direction::Left);
  output.tick(0);
  TEST_ASSERT_EQUAL(config::kServoLeftAngleDeg, servo.last());

  output.tick(config::kServoTurnPulseMs);
  TEST_ASSERT_EQUAL(config::kServoNeutralAngleDeg, servo.last());
}

void test_output_switching_turn_direction_restarts_the_pulse(void) {
  FakeMotorDriver motor;
  FakeSteeringServo servo;
  MotorServoVehicleOutput output(motor, servo);

  output.emit(Direction::Left);
  output.tick(0);
  TEST_ASSERT_EQUAL(config::kServoLeftAngleDeg, servo.last());

  // Switch to Right just before the Left pulse would have expired.
  output.emit(Direction::Right);
  output.tick(config::kServoTurnPulseMs - 1);
  TEST_ASSERT_EQUAL(config::kServoRightAngleDeg, servo.last());

  // The new pulse must run its own full duration from when it started, not
  // inherit whatever time remained on the old one.
  output.tick(2 * (config::kServoTurnPulseMs - 1));
  TEST_ASSERT_EQUAL(config::kServoRightAngleDeg, servo.last());

  output.tick(config::kServoTurnPulseMs - 1 + config::kServoTurnPulseMs);
  TEST_ASSERT_EQUAL(config::kServoNeutralAngleDeg, servo.last());
}

void test_output_stop_centers_servo_immediately_mid_turn_pulse(void) {
  FakeMotorDriver motor;
  FakeSteeringServo servo;
  MotorServoVehicleOutput output(motor, servo);

  output.emit(Direction::Right);
  output.tick(0);
  TEST_ASSERT_EQUAL(config::kServoRightAngleDeg, servo.last());

  // Direction returns to Stop well before the pulse would naturally elapse
  // — the fail-safe/neutral case must center immediately, not wait out the
  // turn pulse.
  output.emit(Direction::Stop);
  output.tick(10);
  TEST_ASSERT_EQUAL(config::kServoNeutralAngleDeg, servo.last());
}
