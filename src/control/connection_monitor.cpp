#include "control/connection_monitor.h"

ConnectionEvent ConnectionMonitor::onTick(bool connected) {
  if (connected == connected_) return ConnectionEvent::None;
  connected_ = connected;
  return connected ? ConnectionEvent::Connected : ConnectionEvent::Disconnected;
}
