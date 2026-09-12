#include <unity.h>

#include "control/lights_control.h"

// User Story 1: LightsControl tracks the four lights' shared ON/OFF state
// (the app has one toggle, so a command always sets all four together),
// defaulting all-OFF (FR-006), and reports whether a command actually
// changed anything (FR-004's "whenever state changes").

void test_lights_control_defaults_all_off(void) {
  LightsControl control;
  LightsState state = control.state();
  for (bool on : state) {
    TEST_ASSERT_FALSE(on);
  }
}

void test_lights_control_on_command_turns_all_lights_on(void) {
  LightsControl control;
  LightsState state;

  bool changed = control.apply({true}, state);
  TEST_ASSERT_TRUE(changed);
  for (bool on : state) {
    TEST_ASSERT_TRUE(on);
  }
}

void test_lights_control_off_command_turns_all_lights_off(void) {
  LightsControl control;
  LightsState state;

  control.apply({true}, state);
  bool changed = control.apply({false}, state);
  TEST_ASSERT_TRUE(changed);
  for (bool on : state) {
    TEST_ASSERT_FALSE(on);
  }
}

void test_lights_control_repeat_command_is_a_no_op(void) {
  LightsControl control;
  LightsState state;

  bool firstChange = control.apply({true}, state);
  TEST_ASSERT_TRUE(firstChange);

  bool secondChange = control.apply({true}, state);
  TEST_ASSERT_FALSE(secondChange);
  for (bool on : state) {
    TEST_ASSERT_TRUE(on);
  }
}

void test_lights_control_state_reflects_last_applied_command(void) {
  LightsControl control;
  LightsState state;

  control.apply({true}, state);
  control.apply({false}, state);

  LightsState current = control.state();
  for (bool on : current) {
    TEST_ASSERT_FALSE(on);
  }
}
