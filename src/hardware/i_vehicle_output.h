#pragma once

#include "control/direction.h"

// Hardware layer interface. This slice's only real implementation logs the
// direction to serial — no servo/DC-motor I/O — but Control never knows or
// cares which concrete implementation it's talking to.
class IVehicleOutput {
 public:
  virtual ~IVehicleOutput() = default;

  virtual void emit(Direction direction) = 0;

  // Periodic hardware update, called once per main loop() iteration
  // regardless of whether a new Direction was just emitted. Implementations
  // that need to advance time-based state (e.g. a motor reversal pause)
  // override this; the default is a no-op for implementations that don't.
  virtual void tick(unsigned long nowMs) {}
};
