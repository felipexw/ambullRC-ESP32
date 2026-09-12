#pragma once

#include <Arduino.h>

#include "config.h"
#include "hardware/i_lights_output.h"

// ESP32 real implementation: drives the four auxiliary lights' GPIO signals
// from the current LightsState, per
// specs/006-four-led-lights-control/contracts/lights-state-to-gpio-contract.md.
// Swappable behind ILightsOutput for tests, like GpioMotorDriver/
// LedConnectionOutput.
class GpioLightsOutput : public ILightsOutput {
 public:
  void begin() {
    for (int i = 0; i < config::kLightCount; i++) {
      pinMode(pin(i), OUTPUT);
      digitalWrite(pin(i), LOW);
    }
  }

  void apply(const LightsState& state) override {
    for (int i = 0; i < config::kLightCount; i++) {
      digitalWrite(pin(i), state[i] ? HIGH : LOW);
    }
  }

 private:
  static int pin(int index) {
    switch (index) {
      case 0: return config::kLight1Pin;
      case 1: return config::kLight2Pin;
      case 2: return config::kLight3Pin;
      default: return config::kLight4Pin;
    }
  }
};
