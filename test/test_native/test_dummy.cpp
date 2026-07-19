#include <unity.h>

// Toolchain smoke test: confirms the native/host test environment runs
// without a physical ESP32, per Constitution Principle II.

void test_host_pipeline_runs(void) {
  TEST_ASSERT_EQUAL(2, 1 + 1);
}
