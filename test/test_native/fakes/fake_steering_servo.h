#pragma once

#include <vector>

#include "hardware/i_steering_servo.h"

// Test double for ISteeringServo: records every angle set, in order.
class FakeSteeringServo : public ISteeringServo {
 public:
  void setAngleDeg(int angleDeg) override { angles_.push_back(angleDeg); }

  const std::vector<int>& angles() const { return angles_; }
  int last() const { return angles_.back(); }
  bool empty() const { return angles_.empty(); }

 private:
  std::vector<int> angles_;
};
