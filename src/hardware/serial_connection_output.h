#pragma once

#include <Arduino.h>

#include <string>

#include "control/connection_event.h"
#include "hardware/i_connection_output.h"

// This slice's only real Hardware implementation: prints connect/disconnect
// events, tagged with the peer's identification when known, to serial.
// Swappable behind IConnectionOutput for tests.
class SerialConnectionOutput : public IConnectionOutput {
 public:
  void emit(ConnectionEvent event, const std::string& deviceId) override {
    if (deviceId.empty()) {
      Serial.println(toString(event));
      return;
    }
    Serial.print(toString(event));
    Serial.print(' ');
    Serial.println(deviceId.c_str());
  }
};
