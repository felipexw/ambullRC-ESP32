#pragma once

#include <vector>

#include "hardware/i_lights_output.h"

// Test double for ILightsOutput: records every applied LightsState in order.
class RecordingLightsOutput : public ILightsOutput {
 public:
  void apply(const LightsState& state) override { applied_.push_back(state); }

  const std::vector<LightsState>& applied() const { return applied_; }
  const LightsState& last() const { return applied_.back(); }
  bool empty() const { return applied_.empty(); }

 private:
  std::vector<LightsState> applied_;
};
