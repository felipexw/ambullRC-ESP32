#include "control/lights_control.h"

bool LightsControl::apply(const LightCommand& command, LightsState& outState) {
  bool changed = false;
  for (bool& light : state_) {
    if (light != command.on) changed = true;
    light = command.on;
  }
  outState = state_;
  return changed;
}
