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

ParseResult DriveCommandAssembler::apply(const std::string& line, unsigned long nowMs,
                                         DriveCommand& out) {
  const std::string word = toUpper(line);

  if (word == "UP") {
    throttle_ = config::kThrottleMax;
    throttleAtMs_ = nowMs;
  } else if (word == "DOWN") {
    throttle_ = config::kThrottleMin;
    throttleAtMs_ = nowMs;
  } else if (word == "LEFT") {
    steer_ = config::kSteerMin;
    steerAtMs_ = nowMs;
  } else if (word == "RIGHT") {
    steer_ = config::kSteerMax;
    steerAtMs_ = nowMs;
  } else if (word == "STOP") {
    throttle_ = 0;
  } else if (word == "CENTER") {
    steer_ = 0;
  } else {
    DriveCommand parsed;
    ParseResult result = parseLine(line, parsed);
    if (result != ParseResult::Ok) return result;
    steer_ = parsed.steer;
    throttle_ = parsed.throttle;
    steerAtMs_ = nowMs;
    throttleAtMs_ = nowMs;
  }

  out.steer = steer_;
  out.throttle = throttle_;
  return ParseResult::Ok;
}

bool DriveCommandAssembler::expireStaleAxes(unsigned long nowMs, DriveCommand& out) {
  bool expired = false;
  if (steer_ != 0 && nowMs - steerAtMs_ >= config::kCommandTimeoutMs) {
    steer_ = 0;
    expired = true;
  }
  if (throttle_ != 0 && nowMs - throttleAtMs_ >= config::kCommandTimeoutMs) {
    throttle_ = 0;
    expired = true;
  }
  if (!expired) return false;

  out.steer = steer_;
  out.throttle = throttle_;
  return true;
}

void DriveCommandAssembler::reset() {
  steer_ = 0;
  throttle_ = 0;
}
