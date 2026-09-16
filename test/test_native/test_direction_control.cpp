#include <unity.h>

#include "config.h"
#include "control/direction_control.h"

// User Story 1: DriveCommand -> Direction mapping, all 9 cases from
// data-model.md. (Safe-state / connection-loss coverage is added by US2.)

void test_direction_stop(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Stop),
                     static_cast<int>(decideDirection({0, 0})));
}

void test_direction_forward(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Forward),
                     static_cast<int>(decideDirection({0, 50})));
}

void test_direction_backward(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Backward),
                     static_cast<int>(decideDirection({0, -50})));
}

void test_direction_left(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Left),
                     static_cast<int>(decideDirection({-50, 0})));
}

void test_direction_right(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Right),
                     static_cast<int>(decideDirection({50, 0})));
}

void test_direction_forward_left(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::ForwardLeft),
                     static_cast<int>(decideDirection({-50, 50})));
}

void test_direction_forward_right(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::ForwardRight),
                     static_cast<int>(decideDirection({50, 50})));
}

void test_direction_backward_left(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::BackwardLeft),
                     static_cast<int>(decideDirection({-50, -50})));
}

void test_direction_backward_right(void) {
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::BackwardRight),
                     static_cast<int>(decideDirection({50, -50})));
}

// motorEngaged(): true only when the DC motor is actually driving
// (Forward/Backward, with or without steering); steering alone or Stop
// leave it idle — drives the automatic engine tone (main.cpp).

void test_motor_engaged_true_for_forward_and_backward(void) {
  TEST_ASSERT_TRUE(motorEngaged(Direction::Forward));
  TEST_ASSERT_TRUE(motorEngaged(Direction::Backward));
  TEST_ASSERT_TRUE(motorEngaged(Direction::ForwardLeft));
  TEST_ASSERT_TRUE(motorEngaged(Direction::ForwardRight));
  TEST_ASSERT_TRUE(motorEngaged(Direction::BackwardLeft));
  TEST_ASSERT_TRUE(motorEngaged(Direction::BackwardRight));
}

void test_motor_engaged_false_for_stop_and_steering_alone(void) {
  TEST_ASSERT_FALSE(motorEngaged(Direction::Stop));
  TEST_ASSERT_FALSE(motorEngaged(Direction::Left));
  TEST_ASSERT_FALSE(motorEngaged(Direction::Right));
}

// User Story 2: safe-state transitions on disconnect/timeout, STOP emitted
// exactly once per transition, auto-resume on the next valid command.

void test_control_onCommand_returns_decided_direction(void) {
  DirectionControl control;
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Forward),
                     static_cast<int>(control.onCommand({0, 50}, 1000)));
}

void test_control_onTick_stops_once_on_disconnect(void) {
  DirectionControl control;
  control.onCommand({0, 50}, 1000);

  Direction out;
  TEST_ASSERT_TRUE(control.onTick(/*connected=*/false, 1010, out));
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Stop), static_cast<int>(out));

  // Still disconnected: no repeated emission.
  TEST_ASSERT_FALSE(control.onTick(/*connected=*/false, 1020, out));
}

void test_control_onTick_stops_once_on_timeout(void) {
  DirectionControl control;
  control.onCommand({0, 50}, 1000);

  Direction out;
  const unsigned long pastTimeout = 1000 + config::kCommandTimeoutMs + 1;
  TEST_ASSERT_TRUE(control.onTick(/*connected=*/true, pastTimeout, out));
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Stop), static_cast<int>(out));

  // Still no new command: no repeated emission.
  TEST_ASSERT_FALSE(control.onTick(/*connected=*/true, pastTimeout + 10, out));
}

void test_control_onTick_no_emission_while_active_and_within_timeout(void) {
  DirectionControl control;
  control.onCommand({0, 50}, 1000);

  Direction out;
  TEST_ASSERT_FALSE(control.onTick(/*connected=*/true, 1000 + config::kCommandTimeoutMs - 1, out));
}

void test_control_resumes_automatically_on_next_valid_command(void) {
  DirectionControl control;
  control.onCommand({0, 50}, 1000);

  Direction out;
  control.onTick(/*connected=*/false, 1010, out);  // -> safe state

  // A new valid command resumes normal control without a separate re-arm step.
  Direction resumed = control.onCommand({50, 0}, 2000);
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Right), static_cast<int>(resumed));

  // And ticking again (connected, within timeout) emits nothing new.
  TEST_ASSERT_FALSE(control.onTick(/*connected=*/true, 2010, out));
}
