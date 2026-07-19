#pragma once

#include <Arduino.h>

#include "control/connection_event.h"
#include "hardware/i_connection_output.h"

// This slice's only real Hardware implementation: prints connect/disconnect
// events to serial. Swappable behind IConnectionOutput for tests.
class SerialConnectionOutput : public IConnectionOutput {
 public:
  void emit(ConnectionEvent event) override { Serial.println(toString(event)); }
};
