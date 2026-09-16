#include <unity.h>

#include "control/tone_control.h"
#include "fakes/fake_tone_output.h"
#include "fakes/fake_transport.h"
#include "protocol/tone_command_parser.h"

// Integration: FakeTransport -> parseHornCommand -> ToneControl ->
// FakeToneOutput. A valid HORN trigger plays when the horn isn't already
// busy; a retrigger while busy is ignored; once no longer busy, the next
// trigger plays again (FR-004/FR-005).

namespace {

// Mirrors main.cpp's intended dispatch: try the horn parser; only a
// recognized trigger word reaches ToneControl/IToneOutput here.
void pumpToneCommand(FakeTransport& transport, ToneControl& control, FakeToneOutput& output) {
  std::string line;
  if (!transport.readLine(line)) return;
  if (parseHornCommand(line) != ParseResult::Ok) return;
  if (control.apply(output.hornBusy())) output.playHorn();
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
