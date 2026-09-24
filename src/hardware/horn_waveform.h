#pragma once

#include <atomic>
#include <cstdint>

#include "config.h"

// The horn's square wave as a sequence of 8-bit DAC levels, one per
// half-period (config::kHornFreqHz, for config::kHornDurationMs). Pure — no
// Arduino/ESP-IDF calls — so it's unit-tested on the host; DacToneOutput
// calls step() from a periodic background timer and writes the returned
// level to the DAC.
//
// start() only arms the tone and returns immediately, so loop() keeps
// handling drive/steering commands while the horn plays (spec 007 FR-007).
//
// start()/busy() run on the Arduino loop task while step() runs on the
// timer task, so the remaining count is atomic. start() is only called while
// !busy() (ToneControl), when step() never writes the counter — so the two
// never race on it.
class HornWaveform {
 public:
  static constexpr uint8_t kMidline = 128;  // silence: DAC output at rest
  static constexpr uint8_t kHigh = 200;
  static constexpr uint8_t kLow = 50;

  static constexpr int kHalfPeriodUs = 1000000 / config::kHornFreqHz / 2;
  static constexpr int kHalfPeriods =
      static_cast<int>(config::kHornDurationMs * 1000 / kHalfPeriodUs) / 2 * 2;  // whole cycles

  void start() { remaining_.store(kHalfPeriods); }

  bool busy() const { return remaining_.load() > 0; }

  // Advances one half-period and returns the DAC level to hold for it:
  // alternating high/low while playing, then the silent midline.
  uint8_t step() {
    int remaining = remaining_.load();
    if (remaining <= 0) return kMidline;
    remaining_.store(remaining - 1);
    return remaining % 2 == 0 ? kHigh : kLow;
  }

 private:
  std::atomic<int> remaining_{0};
};
