#include <unity.h>

#include "control/connection_monitor.h"
#include "control/lights_control.h"
#include "fakes/fake_transport.h"
#include "fakes/recording_lights_output.h"
#include "protocol/light_command_parser.h"
#include "protocol/lights_report_formatter.h"

// Integration tests: FakeTransport -> parseLightCommand -> LightsControl ->
// RecordingLightsOutput (physical effect) and, when the command actually
// changes state, a formatLightsReport() line written back to FakeTransport.
// US1: a light command (the app's single toggle) sets all four lights
//      together; a non-light line leaves the lights untouched.
// US2: a state-changing command sends exactly one LIGHT_ON/LIGHT_OFF report;
//      a no-op command sends nothing.
// US3: a fresh connection alone (no command) sends the current aggregate
//      state (LIGHT_ON only if all four are on).

namespace {

// Mirrors main.cpp's intended dispatch: try the light parser first; only a
// light command reaches LightsControl/ILightsOutput here. Reports the
// resulting aggregate state back to the app only when a light's actual
// state changed (FR-004).
void pumpLightCommand(FakeTransport& transport, LightsControl& control,
                      RecordingLightsOutput& output) {
  std::string line;
  if (!transport.readLine(line)) return;
  LightCommand cmd;
  if (parseLightCommand(line, cmd) != ParseResult::Ok) return;
  LightsState state;
  bool changed = control.apply(cmd, state);
  output.apply(state);
  if (changed) transport.writeLine(formatLightsReport(state));
}

// Mirrors main.cpp's intended dispatch: on a fresh ConnectionEvent::Connected,
// resync the app with the current full state (FR-005) — independent of
// whether any light command triggered it.
void tickConnection(FakeTransport& transport, ConnectionMonitor& monitor,
                     const LightsControl& control) {
  ConnectionEvent event = monitor.onTick(transport.connected());
  if (event == ConnectionEvent::Connected) {
    transport.writeLine(formatLightsReport(control.state()));
  }
}

}  // namespace

void test_light_toggle_flow_on_command_turns_on_all_lights(void) {
  FakeTransport transport;
  LightsControl control;
  RecordingLightsOutput output;

  transport.enqueueLine("LIGHTS_ON");
  pumpLightCommand(transport, control, output);

  TEST_ASSERT_EQUAL(1, output.applied().size());
  const LightsState& applied = output.last();
  for (bool on : applied) {
    TEST_ASSERT_TRUE(on);
  }
}

void test_light_toggle_flow_non_light_line_does_not_touch_lights(void) {
  FakeTransport transport;
  LightsControl control;
  RecordingLightsOutput output;

  transport.enqueueLine("UP");
  pumpLightCommand(transport, control, output);

  TEST_ASSERT_TRUE(output.empty());
}

void test_light_toggle_flow_sends_report_only_on_change(void) {
  FakeTransport transport;
  LightsControl control;
  RecordingLightsOutput output;

  transport.enqueueLine("LIGHTS_ON");
  pumpLightCommand(transport, control, output);

  TEST_ASSERT_EQUAL(1, transport.written().size());
  TEST_ASSERT_EQUAL_STRING("LIGHT_ON", transport.written()[0].c_str());

  // Re-sending the same command is a no-op: no new report line.
  transport.enqueueLine("LIGHTS_ON");
  pumpLightCommand(transport, control, output);

  TEST_ASSERT_EQUAL(1, transport.written().size());
}

void test_light_toggle_flow_off_command_sends_light_off(void) {
  FakeTransport transport;
  LightsControl control;
  RecordingLightsOutput output;

  transport.enqueueLine("LIGHTS_ON");
  transport.enqueueLine("LIGHTS_OFF");
  pumpLightCommand(transport, control, output);
  pumpLightCommand(transport, control, output);

  TEST_ASSERT_EQUAL(2, transport.written().size());
  TEST_ASSERT_EQUAL_STRING("LIGHT_OFF", transport.written().back().c_str());
}

void test_lights_report_sent_on_connect(void) {
  FakeTransport transport;
  LightsControl control;
  RecordingLightsOutput output;
  ConnectionMonitor monitor;

  transport.enqueueLine("LIGHTS_ON");
  pumpLightCommand(transport, control, output);
  TEST_ASSERT_EQUAL(1, transport.written().size());

  transport.setConnected(true);
  tickConnection(transport, monitor, control);

  TEST_ASSERT_EQUAL(2, transport.written().size());
  TEST_ASSERT_EQUAL_STRING("LIGHT_ON", transport.written()[1].c_str());
}
