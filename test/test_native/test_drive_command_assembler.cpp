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
                     static_cast<int>(assembler.apply("UP", 0, cmd)));
  TEST_ASSERT_EQUAL(config::kThrottleMax, cmd.throttle);
  TEST_ASSERT_EQUAL(0, cmd.steer);
}

void test_assembler_down_sets_throttle_reverse(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("DOWN", 0, cmd);
  TEST_ASSERT_EQUAL(config::kThrottleMin, cmd.throttle);
}

void test_assembler_left_sets_steer_min(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("LEFT", 0, cmd);
  TEST_ASSERT_EQUAL(config::kSteerMin, cmd.steer);
  TEST_ASSERT_EQUAL(0, cmd.throttle);
}

void test_assembler_right_sets_steer_max(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("RIGHT", 0, cmd);
  TEST_ASSERT_EQUAL(config::kSteerMax, cmd.steer);
}

void test_assembler_word_commands_are_case_insensitive(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok),
                     static_cast<int>(assembler.apply("right", 0, cmd)));
  TEST_ASSERT_EQUAL(config::kSteerMax, cmd.steer);
}

// The core bug fix: UP then RIGHT must NOT reset throttle back to 0.
void test_assembler_up_then_right_preserves_throttle(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("UP", 0, cmd);
  assembler.apply("RIGHT", 0, cmd);

  TEST_ASSERT_EQUAL(config::kThrottleMax, cmd.throttle);
  TEST_ASSERT_EQUAL(config::kSteerMax, cmd.steer);
}

// And the reverse: steering must not be reset by a later throttle command.
void test_assembler_right_then_up_preserves_steer(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("RIGHT", 0, cmd);
  assembler.apply("UP", 0, cmd);

  TEST_ASSERT_EQUAL(config::kSteerMax, cmd.steer);
  TEST_ASSERT_EQUAL(config::kThrottleMax, cmd.throttle);
}

void test_assembler_numeric_command_overwrites_both_axes(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("UP", 0, cmd);
  assembler.apply("RIGHT", 0, cmd);

  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok), static_cast<int>(assembler.apply("0,0", 0, cmd)));
  TEST_ASSERT_EQUAL(0, cmd.steer);
  TEST_ASSERT_EQUAL(0, cmd.throttle);
}

// STOP/CENTER are the explicit release signals: pressing a button then
// releasing it must zero only that button's axis, immediately, without
// touching the other axis.

void test_assembler_stop_zeros_throttle_preserves_steer(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("UP", 0, cmd);
  assembler.apply("RIGHT", 0, cmd);

  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok),
                     static_cast<int>(assembler.apply("STOP", 0, cmd)));
  TEST_ASSERT_EQUAL(0, cmd.throttle);
  TEST_ASSERT_EQUAL(config::kSteerMax, cmd.steer);
}

void test_assembler_center_zeros_steer_preserves_throttle(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("UP", 0, cmd);
  assembler.apply("RIGHT", 0, cmd);

  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok),
                     static_cast<int>(assembler.apply("CENTER", 0, cmd)));
  TEST_ASSERT_EQUAL(0, cmd.steer);
  TEST_ASSERT_EQUAL(config::kThrottleMax, cmd.throttle);
}

void test_assembler_rejects_unrecognized_word(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(assembler.apply("SIDEWAYS", 0, cmd)));
}

void test_assembler_reset_clears_stale_axis_state(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("UP", 0, cmd);
  assembler.reset();

  assembler.apply("RIGHT", 0, cmd);
  TEST_ASSERT_EQUAL(config::kSteerMax, cmd.steer);
  TEST_ASSERT_EQUAL(0, cmd.throttle);  // not resurrected from before reset()
}

// Per-axis expiry: the app resends a word for as long as its button is held,
// so an axis that stops being resent was released. It must drop back to
// neutral on its own, even while the OTHER axis keeps being resent.

// Regression: steering held (LEFT resent) while the throttle button is
// released used to leave the throttle latched forever — every LEFT counted
// as a fresh command, so nothing ever timed the stale UP out.
void test_assembler_expires_throttle_no_longer_resent_while_steer_is_held(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("LEFT", 1000, cmd);
  assembler.apply("UP", 1010, cmd);

  // UP is released (never resent); LEFT keeps being resent.
  const unsigned long throttleExpiresAt = 1010 + config::kCommandTimeoutMs;
  assembler.apply("LEFT", throttleExpiresAt - 10, cmd);
  TEST_ASSERT_EQUAL(config::kThrottleMax, cmd.throttle);

  DriveCommand expired;
  TEST_ASSERT_TRUE(assembler.expireStaleAxes(throttleExpiresAt, expired));
  TEST_ASSERT_EQUAL(0, expired.throttle);
  TEST_ASSERT_EQUAL(config::kSteerMin, expired.steer);

  // Already expired: nothing new to report, and the next LEFT must not
  // resurrect the throttle.
  TEST_ASSERT_FALSE(assembler.expireStaleAxes(throttleExpiresAt + 1, expired));
  assembler.apply("LEFT", throttleExpiresAt + 5, cmd);
  TEST_ASSERT_EQUAL(0, cmd.throttle);
}

// The mirror case: throttle held (UP resent) while the steer button is
// released.
void test_assembler_expires_steer_no_longer_resent_while_throttle_is_held(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("UP", 1000, cmd);
  assembler.apply("RIGHT", 1010, cmd);

  const unsigned long steerExpiresAt = 1010 + config::kCommandTimeoutMs;
  assembler.apply("UP", steerExpiresAt - 10, cmd);

  DriveCommand expired;
  TEST_ASSERT_TRUE(assembler.expireStaleAxes(steerExpiresAt, expired));
  TEST_ASSERT_EQUAL(0, expired.steer);
  TEST_ASSERT_EQUAL(config::kThrottleMax, expired.throttle);
}

void test_assembler_keeps_axes_that_are_still_being_resent(void) {
  DriveCommandAssembler assembler;
  DriveCommand cmd;

  assembler.apply("UP", 1000, cmd);
  assembler.apply("RIGHT", 1000, cmd);

  DriveCommand expired;
  TEST_ASSERT_FALSE(assembler.expireStaleAxes(1000 + config::kCommandTimeoutMs - 1, expired));

  assembler.apply("UP", 1000 + config::kCommandTimeoutMs - 1, cmd);
  assembler.apply("RIGHT", 1000 + config::kCommandTimeoutMs - 1, cmd);
  TEST_ASSERT_FALSE(assembler.expireStaleAxes(1000 + config::kCommandTimeoutMs, expired));
  TEST_ASSERT_EQUAL(config::kThrottleMax, cmd.throttle);
  TEST_ASSERT_EQUAL(config::kSteerMax, cmd.steer);
}
