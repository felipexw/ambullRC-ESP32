#pragma once

#include <Arduino.h>

#include "control/direction.h"
#include "hardware/i_vehicle_output.h"

// This slice's only real Hardware implementation: prints the decided
// direction to serial. No servo/DC-motor I/O. Swappable behind
// IVehicleOutput for a future feature that actually drives the motors.
class SerialDirectionOutput : public IVehicleOutput {
 public:
  void emit(Direction direction) override { Serial.println(toString(direction)); }
};
