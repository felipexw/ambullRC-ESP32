#include <unity.h>

#include "config.h"
#include "protocol/drive_command_assembler.h"

// The real Android app sends per-axis word commands ("UP"/"DOWN"/"LEFT"/
// "RIGHT"), not the "<steer>,<throttle>" numeric pairs used for manual
// testing. A word command must only touch its own axis so driving and
// steering stay independent (spec 002 US2), and the numeric format keeps
// working unchanged for manual/terminal testing.

void test_assembler_up_sets_throttle_forward_steer_straight(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok),
                     static_cast<int>(assembler.apply("UP", cmd)));
  TEST_ASSERT_EQUAL(config::kThrottleMax, cmd.throttle);
  TEST_ASSERT_EQUAL(0, cmd.steer);
}

void test_assembler_down_sets_throttle_reverse(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("DOWN", cmd);
  TEST_ASSERT_EQUAL(config::kThrottleMin, cmd.throttle);
}

void test_assembler_left_sets_steer_min(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("LEFT", cmd);
  TEST_ASSERT_EQUAL(config::kSteerMin, cmd.steer);
  TEST_ASSERT_EQUAL(0, cmd.throttle);
}

void test_assembler_right_sets_steer_max(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("RIGHT", cmd);
  TEST_ASSERT_EQUAL(config::kSteerMax, cmd.steer);
}

void test_assembler_word_commands_are_case_insensitive(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok),
                     static_cast<int>(assembler.apply("right", cmd)));
  TEST_ASSERT_EQUAL(config::kSteerMax, cmd.steer);
}

// The core bug fix: UP then RIGHT must NOT reset throttle back to 0.
void test_assembler_up_then_right_preserves_throttle(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("UP", cmd);
  assembler.apply("RIGHT", cmd);

  TEST_ASSERT_EQUAL(config::kThrottleMax, cmd.throttle);
  TEST_ASSERT_EQUAL(config::kSteerMax, cmd.steer);
}

// And the reverse: steering must not be reset by a later throttle command.
void test_assembler_right_then_up_preserves_steer(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("RIGHT", cmd);
  assembler.apply("UP", cmd);

  TEST_ASSERT_EQUAL(config::kSteerMax, cmd.steer);
  TEST_ASSERT_EQUAL(config::kThrottleMax, cmd.throttle);
}

void test_assembler_numeric_command_overwrites_both_axes(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("UP", cmd);
  assembler.apply("RIGHT", cmd);

  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok), static_cast<int>(assembler.apply("0,0", cmd)));
  TEST_ASSERT_EQUAL(0, cmd.steer);
  TEST_ASSERT_EQUAL(0, cmd.throttle);
}

void test_assembler_rejects_unrecognized_word(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(assembler.apply("SIDEWAYS", cmd)));
}

void test_assembler_reset_clears_stale_axis_state(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("UP", cmd);
  assembler.reset();

  assembler.apply("RIGHT", cmd);
  TEST_ASSERT_EQUAL(config::kSteerMax, cmd.steer);
  TEST_ASSERT_EQUAL(0, cmd.throttle);  // not resurrected from before reset()
}
