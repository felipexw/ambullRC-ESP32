#pragma once

#include <array>
#include <string>

#include "config.h"
#include "protocol/command_parser.h"

// Auxiliary light command protocol — see
// specs/006-four-led-lights-control/contracts/light-command-protocol.md

struct LightCommand {
  bool on = false;  // applies to all lights together — the app has one toggle
};

// Current ON/OFF state of all lights together, indexed 0..kLightCount-1.
using LightsState = std::array<bool, config::kLightCount>;

// Parses one "LIGHTS_ON"/"LIGHTS_OFF" line, case-insensitive. On
// ParseResult::Ok, `out` holds the parsed command; otherwise `out` is left
// unchanged. Any line that isn't this exact shape (including every existing
// drive word/numeric pair) is ParseResult::Malformed, so the caller can fall
// through to DriveCommandAssembler unchanged — see research.md §3.
ParseResult parseLightCommand(const std::string& line, LightCommand& out);
