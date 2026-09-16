#include <unity.h>

#include "control/tone_control.h"

// ToneControl honors a horn trigger only when the horn isn't already
// playing; otherwise it's ignored (FR-004/FR-005) until it finishes.

void test_tone_control_honors_request_when_not_busy(void) {
  ToneControl control;
  TEST_ASSERT_TRUE(control.apply(/*hornBusy=*/false));
}

void test_tone_control_ignores_request_while_busy(void) {
  ToneControl control;
  TEST_ASSERT_FALSE(control.apply(/*hornBusy=*/true));
}

void test_tone_control_honors_request_again_once_no_longer_busy(void) {
  ToneControl control;
  TEST_ASSERT_FALSE(control.apply(/*hornBusy=*/true));
  TEST_ASSERT_TRUE(control.apply(/*hornBusy=*/false));
}
