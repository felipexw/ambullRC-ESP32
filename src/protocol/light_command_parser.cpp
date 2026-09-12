#include "protocol/light_command_parser.h"

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

ParseResult parseLightCommand(const std::string& line, LightCommand& out) {
  const std::string word = toUpper(line);

  if (word == "LIGHTS_ON") {
    out.on = true;
    return ParseResult::Ok;
  }
  if (word == "LIGHTS_OFF") {
    out.on = false;
    return ParseResult::Ok;
  }
  return ParseResult::Malformed;
}
