#pragma once

#include <vector>

#include "hardware/i_vehicle_output.h"

// Test double for IVehicleOutput: records every emitted Direction in order.
class RecordingVehicleOutput : public IVehicleOutput {
 public:
  void emit(Direction direction) override { emitted_.push_back(direction); }

  const std::vector<Direction>& emitted() const { return emitted_; }
  Direction last() const { return emitted_.back(); }
  bool empty() const { return emitted_.empty(); }

 private:
  std::vector<Direction> emitted_;
};
