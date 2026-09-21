#pragma once

// Sound-effect output: a one-shot horn trigger. The output is silent
// whenever the horn isn't playing.
class IToneOutput {
 public:
  virtual ~IToneOutput() = default;

  // Starts the fixed-duration horn tone; a call while the horn is already
  // playing is a no-op (ToneControl is expected to check hornBusy() first).
  virtual void playHorn() = 0;

  // True for the duration of an in-progress horn play.
  virtual bool hornBusy() = 0;

  virtual void tick(unsigned long nowMs) = 0;
};
