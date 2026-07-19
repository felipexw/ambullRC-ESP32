#pragma once

#include <string>

#include "control/connection_event.h"

// Hardware layer interface for connection lifecycle logging. This slice's
// only real implementation logs to serial, but Control never knows or
// cares which concrete implementation it's talking to.
class IConnectionOutput {
 public:
  virtual ~IConnectionOutput() = default;

  // `deviceId` identifies the peer the event pertains to (see
  // ITransport::deviceId) — empty if unknown.
  virtual void emit(ConnectionEvent event, const std::string& deviceId) = 0;
};
