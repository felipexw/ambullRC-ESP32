#pragma once

// Hardware layer interface for the steering servo.
class ISteeringServo {
 public:
  virtual ~ISteeringServo() = default;

  virtual void setAngleDeg(int angleDeg) = 0;
};
