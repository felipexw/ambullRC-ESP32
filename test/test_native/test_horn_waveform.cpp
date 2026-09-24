#include <unity.h>

#include "config.h"
#include "hardware/horn_waveform.h"

// HornWaveform is the step-by-step horn square wave DacToneOutput drives
// from a background timer, so playHorn() returns immediately instead of
// blocking loop() — and with it every steering/drive command — for the
// horn's whole duration (regression: spec 007 FR-007).

void test_horn_waveform_idle_is_silent_and_not_busy(void) {
  HornWaveform wave;
  TEST_ASSERT_FALSE(wave.busy());
  TEST_ASSERT_EQUAL_UINT8(HornWaveform::kMidline, wave.step());
  TEST_ASSERT_FALSE(wave.busy());
}

void test_horn_waveform_start_returns_immediately_and_is_busy(void) {
  HornWaveform wave;
  wave.start();  // arms the tone only; no level has been produced yet
  TEST_ASSERT_TRUE(wave.busy());
}

void test_horn_waveform_alternates_high_and_low_while_playing(void) {
  HornWaveform wave;
  wave.start();
  TEST_ASSERT_EQUAL_UINT8(HornWaveform::kHigh, wave.step());
  TEST_ASSERT_EQUAL_UINT8(HornWaveform::kLow, wave.step());
  TEST_ASSERT_EQUAL_UINT8(HornWaveform::kHigh, wave.step());
  TEST_ASSERT_EQUAL_UINT8(HornWaveform::kLow, wave.step());
  TEST_ASSERT_TRUE(wave.busy());
}

void test_horn_waveform_ends_silent_after_full_duration(void) {
  HornWaveform wave;
  wave.start();
  for (int i = 0; i < HornWaveform::kHalfPeriods; i++) {
    TEST_ASSERT_TRUE(wave.busy());
    TEST_ASSERT_NOT_EQUAL(HornWaveform::kMidline, wave.step());
  }
  TEST_ASSERT_FALSE(wave.busy());
  TEST_ASSERT_EQUAL_UINT8(HornWaveform::kMidline, wave.step());
}

void test_horn_waveform_duration_matches_config(void) {
  // kHalfPeriods steps of kHalfPeriodUs each span the configured horn
  // duration (to within one half-period of integer rounding).
  const long playedUs = static_cast<long>(HornWaveform::kHalfPeriods) * HornWaveform::kHalfPeriodUs;
  const long wantUs = static_cast<long>(config::kHornDurationMs) * 1000;
  TEST_ASSERT_TRUE(playedUs <= wantUs);
  TEST_ASSERT_TRUE(wantUs - playedUs < 2 * HornWaveform::kHalfPeriodUs);
  TEST_ASSERT_EQUAL(1000000 / config::kHornFreqHz / 2, HornWaveform::kHalfPeriodUs);
}

void test_horn_waveform_can_play_again_once_finished(void) {
  HornWaveform wave;
  wave.start();
  for (int i = 0; i < HornWaveform::kHalfPeriods; i++) wave.step();
  TEST_ASSERT_FALSE(wave.busy());

  wave.start();
  TEST_ASSERT_TRUE(wave.busy());
  TEST_ASSERT_EQUAL_UINT8(HornWaveform::kHigh, wave.step());
}
