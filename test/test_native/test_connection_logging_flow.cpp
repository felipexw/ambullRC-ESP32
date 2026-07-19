#include <unity.h>

#include "control/connection_monitor.h"
#include "fakes/fake_transport.h"
#include "fakes/recording_connection_output.h"

// Integration test: FakeTransport -> ConnectionMonitor -> RecordingConnectionOutput.
// A successful connection and a lost connection each log exactly once.

namespace {

void connectionTick(FakeTransport& transport, ConnectionMonitor& monitor,
                     RecordingConnectionOutput& output) {
  ConnectionEvent event = monitor.onTick(transport.connected());
  if (event != ConnectionEvent::None) output.emit(event);
}

}  // namespace

void test_flow_logs_connected_then_disconnected_then_reconnected(void) {
  FakeTransport transport;
  ConnectionMonitor monitor;
  RecordingConnectionOutput output;

  transport.setConnected(true);
  connectionTick(transport, monitor, output);
  TEST_ASSERT_EQUAL(1, output.emitted().size());
  TEST_ASSERT_EQUAL(static_cast<int>(ConnectionEvent::Connected),
                     static_cast<int>(output.last()));

  connectionTick(transport, monitor, output);  // still connected: no repeat
  TEST_ASSERT_EQUAL(1, output.emitted().size());

  transport.setConnected(false);
  connectionTick(transport, monitor, output);
  TEST_ASSERT_EQUAL(2, output.emitted().size());
  TEST_ASSERT_EQUAL(static_cast<int>(ConnectionEvent::Disconnected),
                     static_cast<int>(output.last()));

  transport.setConnected(true);
  connectionTick(transport, monitor, output);
  TEST_ASSERT_EQUAL(3, output.emitted().size());
  TEST_ASSERT_EQUAL(static_cast<int>(ConnectionEvent::Connected),
                     static_cast<int>(output.last()));
}
