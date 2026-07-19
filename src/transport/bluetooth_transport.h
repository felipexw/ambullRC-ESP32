#pragma once

#include <BluetoothSerial.h>

#include <string>

#include "transport/i_transport.h"

// ESP32-only. Never included by test/test_native — keeps the host test
// build free of Arduino/BluetoothSerial headers (Constitution Principle II).
class BluetoothTransport : public ITransport {
 public:
  void begin(const char* deviceName) { bt_.begin(deviceName); }

  bool connected() override { return bt_.hasClient(); }

  bool readLine(std::string& outLine) override {
    while (bt_.available()) {
      char c = static_cast<char>(bt_.read());
      if (c == '\n') {
        outLine = buffer_;
        buffer_.clear();
        return true;
      }
      if (c != '\r') buffer_ += c;
    }
    return false;
  }

 private:
  BluetoothSerial bt_;
  std::string buffer_;
};
