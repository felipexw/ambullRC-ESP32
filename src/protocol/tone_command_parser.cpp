#include "protocol/tone_command_parser.h"

#include <algorithm>
#include <cctype>

namespace {

std::string toUpper(const std::string& s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(),
                  [](unsigned char c) { return std::toupper(c); });
  return out;
}

}  // namespace

ParseResult parseHornCommand(const std::string& line) {
  return toUpper(line) == "HORN" ? ParseResult::Ok : ParseResult::Malformed;
}
