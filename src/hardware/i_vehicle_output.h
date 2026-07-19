#pragma once

#include "control/direction.h"

// Hardware layer interface. This slice's only real implementation logs the
// direction to serial — no servo/DC-motor I/O — but Control never knows or
// cares which concrete implementation it's talking to.
class IVehicleOutput {
 public:
  virtual ~IVehicleOutput() = default;

  virtual void emit(Direction direction) = 0;
};
