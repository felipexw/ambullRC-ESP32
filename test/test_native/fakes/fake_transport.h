#pragma once

#include <deque>
#include <string>

#include "transport/i_transport.h"

// Test double for ITransport: queue lines to be "received", toggle the
// connected flag to simulate a Bluetooth disconnect.
class FakeTransport : public ITransport {
 public:
  bool connected() override { return connected_; }

  std::string deviceId() override { return deviceId_; }

  bool readLine(std::string& outLine) override {
    if (queue_.empty()) return false;
    outLine = queue_.front();
    queue_.pop_front();
    return true;
  }

  void enqueueLine(const std::string& line) { queue_.push_back(line); }
  void setConnected(bool connected) { connected_ = connected; }
  void setDeviceId(const std::string& deviceId) { deviceId_ = deviceId; }

 private:
  std::deque<std::string> queue_;
  bool connected_ = true;
  std::string deviceId_;
};
