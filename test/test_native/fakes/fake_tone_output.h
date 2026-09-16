#pragma once

#include <vector>

#include "hardware/i_tone_output.h"

// Test double for IToneOutput: records horn plays and every setEngineRunning
// call, and lets tests toggle horn-busy directly rather than simulating real
// timing.
class FakeToneOutput : public IToneOutput {
 public:
  void playHorn() override {
    hornPlayCount_++;
    hornBusy_ = true;
  }

  bool hornBusy() override { return hornBusy_; }

  void setEngineRunning(bool running) override {
    engineRunning_ = running;
    engineRunningHistory_.push_back(running);
  }

  void tick(unsigned long nowMs) override {}

  void setHornBusy(bool busy) { hornBusy_ = busy; }
  int hornPlayCount() const { return hornPlayCount_; }
  bool engineRunning() const { return engineRunning_; }
  const std::vector<bool>& engineRunningHistory() const { return engineRunningHistory_; }

 private:
  bool hornBusy_ = false;
  int hornPlayCount_ = 0;
  bool engineRunning_ = false;
  std::vector<bool> engineRunningHistory_;
};
