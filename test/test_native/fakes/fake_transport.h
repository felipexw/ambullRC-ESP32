#pragma once

#include <deque>
#include <string>
#include <vector>

#include "transport/i_transport.h"

// Test double for ITransport: queue lines to be "received", toggle the
// connected flag to simulate a Bluetooth disconnect, and record every line
// written back out (see ITransport::writeLine).
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

  void writeLine(const std::string& line) override { written_.push_back(line); }

  void enqueueLine(const std::string& line) { queue_.push_back(line); }
  void setConnected(bool connected) { connected_ = connected; }
  void setDeviceId(const std::string& deviceId) { deviceId_ = deviceId; }
  const std::vector<std::string>& written() const { return written_; }

 private:
  std::deque<std::string> queue_;
  bool connected_ = true;
  std::string deviceId_;
  std::vector<std::string> written_;
};
