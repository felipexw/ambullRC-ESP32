#include "control/direction_control.h"

#include "config.h"

Direction decideDirection(const DriveCommand& command) {
  const bool forward = command.throttle > 0;
  const bool backward = command.throttle < 0;
  const bool left = command.steer < 0;
  const bool right = command.steer > 0;

  if (forward && left) return Direction::ForwardLeft;
  if (forward && right) return Direction::ForwardRight;
  if (backward && left) return Direction::BackwardLeft;
  if (backward && right) return Direction::BackwardRight;
  if (forward) return Direction::Forward;
  if (backward) return Direction::Backward;
  if (left) return Direction::Left;
  if (right) return Direction::Right;
  return Direction::Stop;
}

Direction DirectionControl::onCommand(const DriveCommand& command, unsigned long nowMs) {
  active_ = true;
  lastCommandAtMs_ = nowMs;
  return decideDirection(command);
}

bool DirectionControl::onTick(bool connected, unsigned long nowMs, Direction& outDirection) {
  if (!active_) return false;

  const bool timedOut = (nowMs - lastCommandAtMs_) >= config::kCommandTimeoutMs;
  if (!connected || timedOut) {
    active_ = false;
    outDirection = Direction::Stop;
    return true;
  }
  return false;
}
