#include <unity.h>

#include "protocol/lights_report_formatter.h"

// User Story 2: LightsState -> aggregate "LIGHT_ON"/"LIGHT_OFF" wire report.
// The app only needs to know whether every light is on — individual
// per-light state is not exposed on the wire.

void test_format_all_off_is_light_off(void) {
  LightsState state = {false, false, false, false};
  TEST_ASSERT_EQUAL_STRING("LIGHT_OFF", formatLightsReport(state).c_str());
}

void test_format_all_on_is_light_on(void) {
  LightsState state = {true, true, true, true};
  TEST_ASSERT_EQUAL_STRING("LIGHT_ON", formatLightsReport(state).c_str());
}

void test_format_single_light_on_is_light_off(void) {
  LightsState state = {true, false, false, false};
  TEST_ASSERT_EQUAL_STRING("LIGHT_OFF", formatLightsReport(state).c_str());
}

void test_format_three_of_four_on_is_still_light_off(void) {
  LightsState state = {true, true, true, false};
  TEST_ASSERT_EQUAL_STRING("LIGHT_OFF", formatLightsReport(state).c_str());
}
