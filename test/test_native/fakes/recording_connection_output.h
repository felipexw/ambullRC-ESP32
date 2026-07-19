#pragma once

#include <vector>

#include "hardware/i_connection_output.h"

// Test double for IConnectionOutput: records every emitted ConnectionEvent in order.
class RecordingConnectionOutput : public IConnectionOutput {
 public:
  void emit(ConnectionEvent event) override { emitted_.push_back(event); }

  const std::vector<ConnectionEvent>& emitted() const { return emitted_; }
  ConnectionEvent last() const { return emitted_.back(); }
  bool empty() const { return emitted_.empty(); }

 private:
  std::vector<ConnectionEvent> emitted_;
};
