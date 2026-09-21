#pragma once

#include <Arduino.h>

#include "config.h"
#include "hardware/i_tone_output.h"

// ESP32 real implementation: drives config::kToneOutputPin via the LEDC
// tone-generation peripheral (the same PWM hardware family PwmSteeringServo
// uses). Plays the horn frequency for config::kHornDurationMs after
// playHorn(), and is silent otherwise.
class PwmToneOutput : public IToneOutput {
 public:
  void begin() {
    ledcSetup(config::kToneLedcChannel, config::kHornFreqHz, kResolutionBits);
    ledcAttachPin(config::kToneOutputPin, config::kToneLedcChannel);
    ledcWriteTone(config::kToneLedcChannel, 0);
  }

  void playHorn() override {
    hornActive_ = true;
    hornStartMs_ = millis();
    Serial.println("playing horn at ");
    Serial.print(config::kHornFreqHz);
    ledcWriteTone(config::kToneLedcChannel, config::kHornFreqHz);
  }

  bool hornBusy() override { return hornActive_; }

  void tick(unsigned long nowMs) override {
    if (hornActive_ && (nowMs - hornStartMs_) >= config::kHornDurationMs) {
      hornActive_ = false;
      ledcWriteTone(config::kToneLedcChannel, 0);
    }
  }

 private:
  static constexpr int kResolutionBits = 8;

  bool hornActive_ = false;
  unsigned long hornStartMs_ = 0;
};
