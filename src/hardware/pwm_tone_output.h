#pragma once

#include <Arduino.h>

#include "config.h"
#include "hardware/i_tone_output.h"

// ESP32 real implementation: drives config::kToneOutputPin via the LEDC
// tone-generation peripheral (the same PWM hardware family PwmSteeringServo
// uses). A single LEDC channel can only produce one frequency at a time, so
// the engine's continuous idle/running character comes from wobbling that
// frequency over time, and the horn is layered "on top" of it by rapidly
// time-slicing between the horn frequency and the current engine frequency
// rather than silencing the engine — see
// specs/007-onboard-sound-effects/contracts/tone-output-hardware-contract.md
// and research.md §6/§7.
class PwmToneOutput : public IToneOutput {
 public:
  void begin() {
    ledcSetup(config::kToneLedcChannel, config::kEngineIdleBaseFreqHz, kResolutionBits);
    ledcAttachPin(config::kToneOutputPin, config::kToneLedcChannel);
    // No silent boot state: the engine idles continuously from power-on,
    // like a real car sitting with the engine running (spec Assumptions).
  }

  void playHorn() override {
    hornActive_ = true;
    hornStartMs_ = millis();
  }

  bool hornBusy() override { return hornActive_; }

  void setEngineRunning(bool running) override { engineRunning_ = running; }

  void tick(unsigned long nowMs) override {
    if (hornActive_ && (nowMs - hornStartMs_) >= config::kHornDurationMs) {
      hornActive_ = false;
    }

    int freq;
    if (hornActive_ && isHornSlice(nowMs - hornStartMs_)) {
      freq = config::kHornFreqHz;
    } else {
      freq = engineRunning_ ? runningFrequency(nowMs) : idleFrequency(nowMs);
    }
    ledcWriteTone(config::kToneLedcChannel, freq);
  }

 private:
  static constexpr int kResolutionBits = 8;

  // Within each kToneLayerPeriodMs window, the first kToneLayerHornSliceMs
  // is horn, the rest is engine — fast enough to read as layered rather than
  // as distinct alternating blips.
  static bool isHornSlice(unsigned long hornElapsedMs) {
    return (hornElapsedMs % config::kToneLayerPeriodMs) < config::kToneLayerHornSliceMs;
  }

  // Approximates a V8's lopey burble: wobbles the base firing frequency on
  // two overlaid short, uneven cycles rather than holding a perfectly steady
  // tone (research.md §6).
  static int idleFrequency(unsigned long nowMs) {
    return wobble(nowMs, config::kEngineIdleBaseFreqHz, config::kEngineIdleWobbleFreqHz);
  }

  static int runningFrequency(unsigned long nowMs) {
    return wobble(nowMs, config::kEngineRunningBaseFreqHz, config::kEngineRunningWobbleFreqHz);
  }

  static int wobble(unsigned long nowMs, int baseFreqHz, int wobbleFreqHz) {
    long w = (nowMs % 110 < 55 ? 1 : -1) + (nowMs % 170 < 85 ? 1 : -1);
    return baseFreqHz + static_cast<int>(w * wobbleFreqHz / 2);
  }

  bool hornActive_ = false;
  unsigned long hornStartMs_ = 0;
  bool engineRunning_ = false;
};
