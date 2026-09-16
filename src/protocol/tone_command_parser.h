#pragma once

#include <string>

#include "protocol/command_parser.h"

// The only remaining manual sound-effect trigger word. The engine tone is no
// longer commanded directly — it's fully automatic, driven by drive-command
// state (see IToneOutput::setEngineRunning) — so "ENGINE" (like the earlier
// removed "SIREN") now falls through as Malformed here, same as every other
// word this parser doesn't recognize.
ParseResult parseHornCommand(const std::string& line);
