#pragma once

#include <string>

// Bluetooth command protocol v1 — see
// specs/001-bluetooth-motor-control/contracts/bluetooth-command-protocol.md

struct DriveCommand {
  int steer = 0;
  int throttle = 0;
};

enum class ParseResult {
  Ok,
  Malformed,
  OutOfRange,
};

// Parses one "<steer>,<throttle>" line. On ParseResult::Ok, `out` holds the
// parsed command; otherwise `out` is left unchanged.
ParseResult parseLine(const std::string& line, DriveCommand& out);
