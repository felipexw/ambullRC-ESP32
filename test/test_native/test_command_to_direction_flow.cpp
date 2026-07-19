#include <unity.h>

#include "control/direction_control.h"
#include "fakes/fake_transport.h"
#include "fakes/recording_vehicle_output.h"
#include "protocol/command_parser.h"

// Integration tests: FakeTransport -> parseLine -> DirectionControl ->
// RecordingVehicleOutput.
// US1: valid commands produce the expected Direction.
// US2: disconnect/timeout emit STOP exactly once and auto-resume; malformed
//      lines are discarded without crashing.
// (Out-of-range flows are added by US3.)

namespace {

void pumpCommand(FakeTransport& transport, DirectionControl& control,
                  RecordingVehicleOutput& output, unsigned long nowMs) {
  std::string line;
  if (!transport.readLine(line)) return;
  DriveCommand cmd;
  if (parseLine(line, cmd) != ParseResult::Ok) return;
  output.emit(control.onCommand(cmd, nowMs));
}

void tick(FakeTransport& transport, DirectionControl& control,
          RecordingVehicleOutput& output, unsigned long nowMs) {
  Direction direction;
  if (control.onTick(transport.connected(), nowMs, direction)) {
    output.emit(direction);
  }
}

}  // namespace

void test_flow_valid_commands_produce_expected_directions(void) {
  FakeTransport transport;
  DirectionControl control;
  RecordingVehicleOutput output;

  transport.enqueueLine("0,100");
  transport.enqueueLine("-50,0");
  transport.enqueueLine("40,-30");

  pumpCommand(transport, control, output, 1000);
  pumpCommand(transport, control, output, 1010);
  pumpCommand(transport, control, output, 1020);

  TEST_ASSERT_EQUAL(3, output.emitted().size());
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Forward),
                     static_cast<int>(output.emitted()[0]));
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Left),
                     static_cast<int>(output.emitted()[1]));
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::BackwardRight),
                     static_cast<int>(output.emitted()[2]));
}

void test_flow_disconnect_emits_single_stop_and_resumes(void) {
  FakeTransport transport;
  DirectionControl control;
  RecordingVehicleOutput output;

  transport.enqueueLine("0,100");
  pumpCommand(transport, control, output, 1000);
  TEST_ASSERT_EQUAL(1, output.emitted().size());

  transport.setConnected(false);
  tick(transport, control, output, 1010);
  tick(transport, control, output, 1020);  // still disconnected: no repeat

  TEST_ASSERT_EQUAL(2, output.emitted().size());
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Stop),
                     static_cast<int>(output.emitted()[1]));

  // Reconnect and send a fresh valid command: resumes automatically.
  transport.setConnected(true);
  transport.enqueueLine("50,0");
  pumpCommand(transport, control, output, 2000);

  TEST_ASSERT_EQUAL(3, output.emitted().size());
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Right),
                     static_cast<int>(output.emitted()[2]));
}

void test_flow_malformed_line_does_not_crash_or_emit(void) {
  FakeTransport transport;
  DirectionControl control;
  RecordingVehicleOutput output;

  transport.enqueueLine("not-a-command");
  pumpCommand(transport, control, output, 1000);

  TEST_ASSERT_TRUE(output.empty());
}

void test_flow_out_of_range_command_preserves_last_direction(void) {
  FakeTransport transport;
  DirectionControl control;
  RecordingVehicleOutput output;

  transport.enqueueLine("0,100");  // valid: FORWARD
  pumpCommand(transport, control, output, 1000);
  TEST_ASSERT_EQUAL(1, output.emitted().size());
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Forward),
                     static_cast<int>(output.last()));

  transport.enqueueLine("200,0");  // out of range: rejected, not applied
  pumpCommand(transport, control, output, 1010);

  TEST_ASSERT_EQUAL(1, output.emitted().size());
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Forward),
                     static_cast<int>(output.last()));
}
