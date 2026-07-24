#pragma once

// Hardware layer interface for the L9110S DC motor bridge. Expresses only
// polarity (forward/reverse/stopped) — no speed control.
class IMotorDriver {
 public:
  virtual ~IMotorDriver() = default;

  virtual void driveForward() = 0;
  virtual void driveReverse() = 0;
  virtual void stop() = 0;
};
