#include "protocol/lights_report_formatter.h"

#include <algorithm>

std::string formatLightsReport(const LightsState& state) {
  const bool allOn = std::all_of(state.begin(), state.end(), [](bool on) { return on; });
  return allOn ? "LIGHT_ON" : "LIGHT_OFF";
}
