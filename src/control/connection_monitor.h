#pragma once

#include "control/connection_event.h"

// Pure edge-detection: compares the transport's connected() flag tick over
// tick and reports a ConnectionEvent exactly once per transition. Assumes
// the board boots disconnected, so the first tick observing `true` reports
// Connected.
class ConnectionMonitor {
 public:
  ConnectionEvent onTick(bool connected);

 private:
  bool connected_ = false;
};
