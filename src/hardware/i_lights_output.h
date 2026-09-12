#pragma once

#include "protocol/light_command_parser.h"

// Hardware layer interface for the four auxiliary lights. Control never
// knows or cares which concrete implementation it's talking to.
class ILightsOutput {
 public:
  virtual ~ILightsOutput() = default;

  // Drives all four lights from the full current state, per
  // specs/006-four-led-lights-control/contracts/lights-state-to-gpio-contract.md.
  virtual void apply(const LightsState& state) = 0;
};
