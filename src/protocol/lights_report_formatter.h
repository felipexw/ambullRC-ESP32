#pragma once

#include <string>

#include "protocol/light_command_parser.h"

// Formats the current LightsState as the wire report sent to the app:
// "LIGHT_ON" iff all four lights are ON, "LIGHT_OFF" otherwise. The app only
// ever needs to know whether every light is on — see
// specs/006-four-led-lights-control/contracts/light-command-protocol.md.
std::string formatLightsReport(const LightsState& state);
