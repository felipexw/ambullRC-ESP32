#include <unity.h>

#include "control/connection_monitor.h"

// Unit tests: ConnectionMonitor reports a ConnectionEvent exactly once per
// transition of the transport's connected() flag.

void test_connection_monitor_reports_connected_on_first_connect(void) {
  ConnectionMonitor monitor;

  TEST_ASSERT_EQUAL(static_cast<int>(ConnectionEvent::Connected),
                     static_cast<int>(monitor.onTick(true)));
}

void test_connection_monitor_no_event_while_staying_connected(void) {
  ConnectionMonitor monitor;

  monitor.onTick(true);

  TEST_ASSERT_EQUAL(static_cast<int>(ConnectionEvent::None),
                     static_cast<int>(monitor.onTick(true)));
}

void test_connection_monitor_reports_disconnected_after_connect(void) {
  ConnectionMonitor monitor;

  monitor.onTick(true);

  TEST_ASSERT_EQUAL(static_cast<int>(ConnectionEvent::Disconnected),
                     static_cast<int>(monitor.onTick(false)));
}

void test_connection_monitor_no_event_while_staying_disconnected(void) {
  ConnectionMonitor monitor;

  monitor.onTick(false);

  TEST_ASSERT_EQUAL(static_cast<int>(ConnectionEvent::None),
                     static_cast<int>(monitor.onTick(false)));
}

void test_connection_monitor_reports_connected_again_after_reconnect(void) {
  ConnectionMonitor monitor;

  monitor.onTick(true);
  monitor.onTick(false);

  TEST_ASSERT_EQUAL(static_cast<int>(ConnectionEvent::Connected),
                     static_cast<int>(monitor.onTick(true)));
}
