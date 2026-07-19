#include <unity.h>

#include "protocol/command_parser.h"

// User Story 1: well-formed lines parse into the correct DriveCommand.
// (Malformed-line and out-of-range coverage is added by US2/US3 tests below.)

void test_parses_straight_stopped(void) {
  DriveCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok),
                     static_cast<int>(parseLine("0,0", cmd)));
  TEST_ASSERT_EQUAL(0, cmd.steer);
  TEST_ASSERT_EQUAL(0, cmd.throttle);
}

void test_parses_full_forward(void) {
  DriveCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok),
                     static_cast<int>(parseLine("0,100", cmd)));
  TEST_ASSERT_EQUAL(0, cmd.steer);
  TEST_ASSERT_EQUAL(100, cmd.throttle);
}

void test_parses_negative_steer_and_throttle(void) {
  DriveCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok),
                     static_cast<int>(parseLine("-50,-30", cmd)));
  TEST_ASSERT_EQUAL(-50, cmd.steer);
  TEST_ASSERT_EQUAL(-30, cmd.throttle);
}

void test_parses_right_reverse(void) {
  DriveCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok),
                     static_cast<int>(parseLine("40,-30", cmd)));
  TEST_ASSERT_EQUAL(40, cmd.steer);
  TEST_ASSERT_EQUAL(-30, cmd.throttle);
}

// User Story 2: malformed lines are rejected — discarded, not applied, no crash.

void test_rejects_non_numeric_garbage(void) {
  DriveCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseLine("abc", cmd)));
}

void test_rejects_empty_line(void) {
  DriveCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseLine("", cmd)));
}

void test_rejects_missing_throttle_field(void) {
  DriveCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseLine("10", cmd)));
}

void test_rejects_extra_field(void) {
  DriveCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseLine("1,2,3", cmd)));
}

void test_rejects_trailing_garbage(void) {
  DriveCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Malformed),
                     static_cast<int>(parseLine("1,2x", cmd)));
}

// User Story 3: syntactically valid but out-of-range values are rejected
// distinctly from malformed input (FR-005) — boundary values are accepted.

void test_accepts_steer_and_throttle_at_min_boundary(void) {
  DriveCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok),
                     static_cast<int>(parseLine("-100,-100", cmd)));
}

void test_accepts_steer_and_throttle_at_max_boundary(void) {
  DriveCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::Ok),
                     static_cast<int>(parseLine("100,100", cmd)));
}

void test_rejects_steer_below_min(void) {
  DriveCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::OutOfRange),
                     static_cast<int>(parseLine("-101,0", cmd)));
}

void test_rejects_steer_above_max(void) {
  DriveCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::OutOfRange),
                     static_cast<int>(parseLine("101,0", cmd)));
}

void test_rejects_throttle_below_min(void) {
  DriveCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::OutOfRange),
                     static_cast<int>(parseLine("0,-101", cmd)));
}

void test_rejects_throttle_above_max(void) {
  DriveCommand cmd;
  TEST_ASSERT_EQUAL(static_cast<int>(ParseResult::OutOfRange),
                     static_cast<int>(parseLine("0,101", cmd)));
}
