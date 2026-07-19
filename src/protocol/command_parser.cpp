#include "protocol/command_parser.h"

#include <cctype>
#include <cstdlib>

#include "config.h"

namespace {

bool parseInt(const std::string& s, size_t& pos, int& value) {
  size_t start = pos;
  if (pos < s.size() && s[pos] == '-') pos++;
  size_t digitsStart = pos;
  while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) {
    pos++;
  }
  if (pos == digitsStart) return false;
  value = std::atoi(s.substr(start, pos - start).c_str());
  return true;
}

}  // namespace

ParseResult parseLine(const std::string& line, DriveCommand& out) {
  size_t pos = 0;
  int steer = 0;
  int throttle = 0;

  if (!parseInt(line, pos, steer)) return ParseResult::Malformed;
  if (pos >= line.size() || line[pos] != ',') return ParseResult::Malformed;
  pos++;
  if (!parseInt(line, pos, throttle)) return ParseResult::Malformed;
  if (pos != line.size()) return ParseResult::Malformed;

  if (steer < config::kSteerMin || steer > config::kSteerMax) {
    return ParseResult::OutOfRange;
  }
  if (throttle < config::kThrottleMin || throttle > config::kThrottleMax) {
    return ParseResult::OutOfRange;
  }

  out.steer = steer;
  out.throttle = throttle;
  return ParseResult::Ok;
}
