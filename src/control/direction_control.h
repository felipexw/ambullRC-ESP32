#pragma once

#include "control/direction.h"
#include "protocol/command_parser.h"

// Pure decision logic: DriveCommand -> Direction. No I/O, no hardware calls.
Direction decideDirection(const DriveCommand& command);

// Stateful safe-state tracking on top of decideDirection (FR-007/008/009):
// disconnect or command timeout enters a safe state (Direction::Stop,
// emitted exactly once per transition); the next valid command resumes
// normal control automatically, with no separate re-arm step.
class DirectionControl {
 public:
  // Feed a valid command. Returns the Direction to emit and marks the
  // connection active.
  Direction onCommand(const DriveCommand& command, unsigned long nowMs);

  // Feed a periodic tick of connection status/time (e.g. once per loop
  // iteration). If this tick causes a transition into the safe state
  // (disconnected, or no valid command within the configured timeout),
  // sets `outDirection` to Direction::Stop and returns true — exactly once
  // per transition. Returns false otherwise (nothing new to emit).
  bool onTick(bool connected, unsigned long nowMs, Direction& outDirection);

 private:
  bool active_ = false;
  unsigned long lastCommandAtMs_ = 0;
};
