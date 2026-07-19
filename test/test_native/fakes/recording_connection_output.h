#pragma once

#include <string>
#include <vector>

#include "hardware/i_connection_output.h"

struct ConnectionLogEntry {
  ConnectionEvent event;
  std::string deviceId;
};

// Test double for IConnectionOutput: records every emitted (event, deviceId)
// pair in order.
class RecordingConnectionOutput : public IConnectionOutput {
 public:
  void emit(ConnectionEvent event, const std::string& deviceId) override {
    emitted_.push_back({event, deviceId});
  }

  const std::vector<ConnectionLogEntry>& emitted() const { return emitted_; }
  const ConnectionLogEntry& last() const { return emitted_.back(); }
  bool empty() const { return emitted_.empty(); }

 private:
  std::vector<ConnectionLogEntry> emitted_;
};
