#pragma once

#include "hardware/i_tone_output.h"

// Test double for IToneOutput: records horn plays, and lets tests toggle
// horn-busy directly rather than simulating real timing.
class FakeToneOutput : public IToneOutput {
 public:
  void playHorn() override {
    hornPlayCount_++;
    hornBusy_ = true;
  }

  bool hornBusy() override { return hornBusy_; }

  void tick(unsigned long nowMs) override {}

  void setHornBusy(bool busy) { hornBusy_ = busy; }
  int hornPlayCount() const { return hornPlayCount_; }

 private:
  bool hornBusy_ = false;
  int hornPlayCount_ = 0;
};
