#pragma once

// Sound-effect output. The engine tone is fully automatic — it always plays,
// switching between an idle and a running character via setEngineRunning()
// to reflect the DC motor's current state — while the horn is a one-shot
// trigger layered on top of whatever the engine is currently doing (see
// PwmToneOutput/config::kToneLayerPeriodMs).
class IToneOutput {
 public:
  virtual ~IToneOutput() = default;

  // Starts the fixed-duration horn tone; a call while the horn is already
  // playing is a no-op (ToneControl is expected to check hornBusy() first).
  virtual void playHorn() = 0;

  // True for the duration of an in-progress horn play.
  virtual bool hornBusy() = 0;

  // Reflects whether the DC motor is currently engaged (Direction::
  // motorEngaged()) — selects which continuous engine tone plays.
  virtual void setEngineRunning(bool running) = 0;

  virtual void tick(unsigned long nowMs) = 0;
};
