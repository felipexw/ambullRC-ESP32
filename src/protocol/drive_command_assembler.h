#pragma once

#include <string>

#include "protocol/command_parser.h"

// Wraps the wire-format protocol with support for the Android app's actual
// per-axis word commands ("UP"/"DOWN"/"LEFT"/"RIGHT", case-insensitive),
// alongside the original "<steer>,<throttle>" numeric format (still useful
// for manual testing via a generic SPP terminal app) — see
// contracts/bluetooth-command-protocol.md.
//
// A word command updates only the axis it names (throttle for UP/DOWN,
// steer for LEFT/RIGHT); the other axis keeps its last known value, so e.g.
// driving forward and then steering right doesn't reset the throttle back
// to zero. A numeric command sets both axes explicitly, as before.
class DriveCommandAssembler {
 public:
  // Parses `line` and merges it into the persisted per-axis state,
  // producing the full resulting DriveCommand in `out`. Same ParseResult
  // semantics as parseLine(): Ok on success; Malformed/OutOfRange leave the
  // persisted state (and `out`) unchanged.
  ParseResult apply(const std::string& line, DriveCommand& out);

  // Resets both axes to 0 (stopped/straight). Call this whenever the
  // vehicle enters its fail-safe state, so a stale pre-fail-safe axis value
  // can't be silently resurrected by the next single-axis word command.
  void reset();

 private:
  int steer_ = 0;
  int throttle_ = 0;
};
