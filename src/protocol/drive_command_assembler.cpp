#include "protocol/drive_command_assembler.h"

#include <algorithm>
#include <cctype>

#include "config.h"

namespace {

std::string toUpper(const std::string& s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(),
                  [](unsigned char c) { return std::toupper(c); });
  return out;
}

}  // namespace

ParseResult DriveCommandAssembler::apply(const std::string& line, DriveCommand& out) {
  const std::string word = toUpper(line);

  if (word == "UP") {
    throttle_ = config::kThrottleMax;
  } else if (word == "DOWN") {
    throttle_ = config::kThrottleMin;
  } else if (word == "LEFT") {
    steer_ = config::kSteerMin;
  } else if (word == "RIGHT") {
    steer_ = config::kSteerMax;
  } else {
    DriveCommand parsed;
    ParseResult result = parseLine(line, parsed);
    if (result != ParseResult::Ok) return result;
    steer_ = parsed.steer;
    throttle_ = parsed.throttle;
  }

  out.steer = steer_;
  out.throttle = throttle_;
  return ParseResult::Ok;
}

void DriveCommandAssembler::reset() {
  steer_ = 0;
  throttle_ = 0;
}
