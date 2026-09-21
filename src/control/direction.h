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
