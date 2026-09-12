#pragma once

#include "protocol/light_command_parser.h"

// Pure decision logic, no I/O: tracks the four auxiliary lights' persisted
// ON/OFF state and applies validated LightCommands to it. The app has one
// lights toggle, so a command always sets all four lights together. See
// specs/006-four-led-lights-control/data-model.md.
class LightsControl {
 public:
  // Sets all four lights to `command.on`, writes the full resulting state to
  // `outState`, and returns true iff outState differs from the state before
  // this call (i.e., a report should be sent per FR-004). Repeating the
  // state all four lights are already in returns false and leaves outState
  // equal to the unchanged state.
  bool apply(const LightCommand& command, LightsState& outState);

  // The current state, e.g. for a full resync report on connect (FR-005).
  LightsState state() const { return state_; }

 private:
  LightsState state_{};  // all false — FR-006
};
