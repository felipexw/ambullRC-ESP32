#pragma once

#include <string>

#include "protocol/command_parser.h"

// The only sound-effect trigger word. Removed words (e.g. "SIREN", "ENGINE")
// fall through as Malformed here, same as every other word this parser
// doesn't recognize.
ParseResult parseHornCommand(const std::string& line);
