#pragma once

// Direction the Control layer has decided on, per data-model.md. The
// Hardware layer only ever receives this — no steer/throttle magnitudes.
enum class Direction {
  Stop,
  Forward,
  Backward,
  Left,
  Right,
  ForwardLeft,
  ForwardRight,
  BackwardLeft,
  BackwardRight,
};

// True only when the DC motor is actually driving (Forward/Backward, with or
// without steering) — steering alone (Left/Right) or Stop leave the motor
// idle. Used to drive the automatic engine tone (IToneOutput::
// setEngineRunning): running while the motor is engaged, idle otherwise.
inline bool motorEngaged(Direction direction) {
  switch (direction) {
    case Direction::Forward:
    case Direction::Backward:
    case Direction::ForwardLeft:
    case Direction::ForwardRight:
    case Direction::BackwardLeft:
    case Direction::BackwardRight:
      return true;
    case Direction::Stop:
    case Direction::Left:
    case Direction::Right:
      return false;
  }
  return false;
}

inline const char* toString(Direction direction) {
  switch (direction) {
    case Direction::Stop:
      return "STOP";
    case Direction::Forward:
      return "FORWARD";
    case Direction::Backward:
      return "BACKWARD";
    case Direction::Left:
      return "LEFT";
    case Direction::Right:
      return "RIGHT";
    case Direction::ForwardLeft:
      return "FORWARD_LEFT";
    case Direction::ForwardRight:
      return "FORWARD_RIGHT";
    case Direction::BackwardLeft:
      return "BACKWARD_LEFT";
    case Direction::BackwardRight:
      return "BACKWARD_RIGHT";
  }
  return "STOP";
}
