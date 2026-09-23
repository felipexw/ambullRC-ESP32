#include <unity.h>

#include "control/direction_control.h"
#include "control/tone_control.h"
#include "fakes/fake_tone_output.h"
#include "fakes/fake_transport.h"
#include "fakes/recording_vehicle_output.h"
#include "protocol/drive_command_assembler.h"
#include "protocol/tone_command_parser.h"

// Integration: FakeTransport -> parseHornCommand -> ToneControl ->
// FakeToneOutput. A valid HORN trigger plays when the horn isn't already
// busy; a retrigger while busy is ignored; once no longer busy, the next
// trigger plays again (FR-004/FR-005). Drive commands and the safe-state
// tick are unaffected by a busy horn (008 FR-008/FR-009).

namespace {

// Mirrors main.cpp's intended dispatch: try the horn parser; only a
// recognized trigger word reaches ToneControl/IToneOutput here.
void pumpToneCommand(FakeTransport& transport, ToneControl& control, FakeToneOutput& output) {
  std::string line;
  if (!transport.readLine(line)) return;
  if (parseHornCommand(line) != ParseResult::Ok) return;
  if (control.apply(output.hornBusy())) output.playHorn();
}

// Mirrors main.cpp's dispatch: a horn word goes to the horn path, anything
// else through the drive-command assembler into DirectionControl.
void pumpAnyCommand(FakeTransport& transport, ToneControl& toneControl, FakeToneOutput& tone,
                    DriveCommandAssembler& assembler, DirectionControl& control,
                    RecordingVehicleOutput& vehicle, unsigned long nowMs) {
  std::string line;
  if (!transport.readLine(line)) return;
  if (parseHornCommand(line) == ParseResult::Ok) {
    if (toneControl.apply(tone.hornBusy())) tone.playHorn();
    return;
  }
  DriveCommand cmd;
  if (assembler.apply(line, cmd) == ParseResult::Ok) {
    vehicle.emit(control.onCommand(cmd, nowMs));
  }
}

}  // namespace

void test_tone_flow_valid_trigger_plays_when_not_busy(void) {
  FakeTransport transport;
  ToneControl control;
  FakeToneOutput output;

  transport.enqueueLine("HORN");
  pumpToneCommand(transport, control, output);

  TEST_ASSERT_EQUAL(1, output.hornPlayCount());
}

void test_tone_flow_non_tone_line_does_not_trigger_playback(void) {
  FakeTransport transport;
  ToneControl control;
  FakeToneOutput output;

  transport.enqueueLine("UP");
  pumpToneCommand(transport, control, output);

  TEST_ASSERT_EQUAL(0, output.hornPlayCount());
}

void test_tone_flow_retrigger_while_busy_is_ignored(void) {
  FakeTransport transport;
  ToneControl control;
  FakeToneOutput output;

  transport.enqueueLine("HORN");
  pumpToneCommand(transport, control, output);
  TEST_ASSERT_EQUAL(1, output.hornPlayCount());

  transport.enqueueLine("HORN");
  pumpToneCommand(transport, control, output);  // playHorn() already set busy

  TEST_ASSERT_EQUAL(1, output.hornPlayCount());  // no additional play
}

void test_tone_flow_plays_again_once_no_longer_busy(void) {
  FakeTransport transport;
  ToneControl control;
  FakeToneOutput output;

  transport.enqueueLine("HORN");
  pumpToneCommand(transport, control, output);
  TEST_ASSERT_EQUAL(1, output.hornPlayCount());

  output.setHornBusy(false);
  transport.enqueueLine("HORN");
  pumpToneCommand(transport, control, output);

  TEST_ASSERT_EQUAL(2, output.hornPlayCount());
}

void test_tone_flow_drive_commands_unaffected_while_horn_busy(void) {
  FakeTransport transport;
  ToneControl toneControl;
  FakeToneOutput tone;
  DriveCommandAssembler assembler;
  DirectionControl control;
  RecordingVehicleOutput vehicle;

  transport.enqueueLine("HORN");
  pumpAnyCommand(transport, toneControl, tone, assembler, control, vehicle, 1000);
  TEST_ASSERT_EQUAL(1, tone.hornPlayCount());
  TEST_ASSERT_TRUE(tone.hornBusy());

  transport.enqueueLine("UP");
  transport.enqueueLine("LEFT");
  transport.enqueueLine("STOP");
  pumpAnyCommand(transport, toneControl, tone, assembler, control, vehicle, 1010);
  pumpAnyCommand(transport, toneControl, tone, assembler, control, vehicle, 1020);
  pumpAnyCommand(transport, toneControl, tone, assembler, control, vehicle, 1030);

  // Same directions as with the horn silent: UP, then UP+LEFT, then LEFT
  // alone once the throttle axis is released.
  TEST_ASSERT_EQUAL(3, vehicle.emitted().size());
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Forward), static_cast<int>(vehicle.emitted()[0]));
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::ForwardLeft),
                    static_cast<int>(vehicle.emitted()[1]));
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Left), static_cast<int>(vehicle.emitted()[2]));

  // Drive commands neither retriggered nor cut the horn.
  TEST_ASSERT_EQUAL(1, tone.hornPlayCount());
  TEST_ASSERT_TRUE(tone.hornBusy());
}

void test_tone_flow_disconnect_while_horn_busy_still_stops_motor(void) {
  FakeTransport transport;
  ToneControl toneControl;
  FakeToneOutput tone;
  DriveCommandAssembler assembler;
  DirectionControl control;
  RecordingVehicleOutput vehicle;

  transport.enqueueLine("UP");
  pumpAnyCommand(transport, toneControl, tone, assembler, control, vehicle, 1000);
  transport.enqueueLine("HORN");
  pumpAnyCommand(transport, toneControl, tone, assembler, control, vehicle, 1010);
  TEST_ASSERT_TRUE(tone.hornBusy());

  transport.setConnected(false);
  Direction direction;
  TEST_ASSERT_TRUE(control.onTick(transport.connected(), 1020, direction));
  TEST_ASSERT_EQUAL(static_cast<int>(Direction::Stop), static_cast<int>(direction));
  TEST_ASSERT_FALSE(control.onTick(transport.connected(), 1030, direction));  // once only
}
