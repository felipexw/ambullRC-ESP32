#pragma once

// Connection lifecycle event decided by ConnectionMonitor from the
// transport's connected() signal. The Hardware layer only ever receives
// this — no transport details.
enum class ConnectionEvent {
  None,
  Connected,
  Disconnected,
};

inline const char* toString(ConnectionEvent event) {
  switch (event) {
    case ConnectionEvent::None:
      return "";
    case ConnectionEvent::Connected:
      return "BLUETOOTH_CONNECTED";
    case ConnectionEvent::Disconnected:
      return "BLUETOOTH_DISCONNECTED";
  }
  return "";
}
