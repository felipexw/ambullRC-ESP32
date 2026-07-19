#pragma once

#include <string>

// Transport layer: Bluetooth send/receive only, no parsing/decision logic.
class ITransport {
 public:
  virtual ~ITransport() = default;

  virtual bool connected() = 0;

  // Non-blocking poll: returns true and sets `outLine` if a full line (sans
  // newline) is available this call; returns false otherwise.
  virtual bool readLine(std::string& outLine) = 0;
};
