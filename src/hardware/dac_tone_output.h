#pragma once

#include <Arduino.h>

#include "config.h"
#include "hardware/i_tone_output.h"

// ESP32 real implementation: plays a synthesized horn tone by writing
// directly to the built-in 8-bit DAC on config::kToneOutputPin (GPIO26),
// toggling between two levels at the horn frequency for a fixed duration.
// No I2S, no DMA, no stored sample.
//
// playHorn() blocks for the tone's full duration: nothing else on the ESP32
// runs while it plays. That means there's no separate "busy" window to
// track — the call site can't be re-entered until playHorn() returns, which
// already satisfies the ignore-while-playing rule (spec 007 FR-004) by
// construction.
class DacToneOutput : public IToneOutput {
 public:
  void begin() { dacWrite(config::kToneOutputPin, kMidline); }

  void playHorn() override { playTone(config::kHornFreqHz, config::kHornDurationMs); }

  bool hornBusy() override { return false; }

  void tick(unsigned long /*nowMs*/) override {}

 private:
  static constexpr uint8_t kMidline = 128;  // silence: DAC output at rest
  static constexpr uint8_t kHigh = 200;
  static constexpr uint8_t kLow = 50;

  void playTone(int freqHz, int durationMs) {
    int delayMicros = 1000000 / freqHz / 2;
    int cycles = (durationMs * 1000) / (delayMicros * 2);
    for (int i = 0; i < cycles; i++) {
      dacWrite(config::kToneOutputPin, kHigh);
      delayMicroseconds(delayMicros);
      dacWrite(config::kToneOutputPin, kLow);
      delayMicroseconds(delayMicros);
    }
    dacWrite(config::kToneOutputPin, kMidline);
  }
};
