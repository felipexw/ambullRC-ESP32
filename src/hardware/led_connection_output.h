#pragma once

#include <Arduino.h>

#include <string>

#include "config.h"
#include "control/connection_event.h"
#include "hardware/i_connection_output.h"

// ESP32 real implementation: drives an external LED's GPIO signal from the
// Bluetooth connection status, per
// specs/004-connection-status-led/contracts/connection-status-led-signal-contract.md.
// Swappable behind IConnectionOutput for tests, like SerialConnectionOutput.
class LedConnectionOutput : public IConnectionOutput {
 public:
  void begin() {
    pinMode(config::kLedPin, OUTPUT);
    digitalWrite(config::kLedPin, LOW);
  }

  void emit(ConnectionEvent event, const std::string& deviceId) override {
    switch (event) {
      case ConnectionEvent::Connected:
        digitalWrite(config::kLedPin, HIGH);
        break;
      case ConnectionEvent::Disconnected:
        digitalWrite(config::kLedPin, LOW);
        break;
      case ConnectionEvent::None:
        break;
    }
  }
};
