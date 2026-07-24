#pragma once

#include <vector>

#include "hardware/i_motor_driver.h"

// Test double for IMotorDriver: records every call, in order.
class FakeMotorDriver : public IMotorDriver {
 public:
  enum class Call { Forward, Reverse, Stop };

  void driveForward() override { calls_.push_back(Call::Forward); }
  void driveReverse() override { calls_.push_back(Call::Reverse); }
  void stop() override { calls_.push_back(Call::Stop); }

  const std::vector<Call>& calls() const { return calls_; }
  Call last() const { return calls_.back(); }
  bool empty() const { return calls_.empty(); }

 private:
  std::vector<Call> calls_;
};
