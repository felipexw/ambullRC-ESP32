#include <unity.h>

#include "control/direction_control.h"
#include "fakes/fake_tone_output.h"
#include "fakes/fake_transport.h"
#include "protocol/drive_command_assembler.h"

// Integration: FakeTransport -> DriveCommandAssembler -> DirectionControl ->
// motorEngaged() -> FakeToneOutput::setEngineRunning(). The engine tone is
// fully automatic: idle whenever the DC motor isn't engaged (Stop, or
// steering alone), running whenever it is (UP or DOWN, with or without
// steering) — mirrors main.cpp's wiring exactly.

namespace {

void pumpWordCommand(FakeTransport& transport, DriveCommandAssembler& assembler,
                      DirectionControl& control, FakeToneOutput& toneOutput, unsigned long nowMs) {
  std::string line;
  if (!transport.readLine(line)) return;
  DriveCommand cmd;
  if (assembler.apply(line, cmd) != ParseResult::Ok) return;
  Direction direction = control.onCommand(cmd, nowMs);
  toneOutput.setEngineRunning(motorEngaged(direction));
}

}  // namespace

void test_engine_tone_flow_defaults_to_idle(void) {
  FakeToneOutput toneOutput;
  TEST_ASSERT_FALSE(toneOutput.engineRunning());
}

void test_engine_tone_flow_forward_sets_engine_running(void) {
  FakeTransport transport;
  DriveCommandAssembler assembler;
  DirectionControl control;
  FakeToneOutput toneOutput;

  transport.enqueueLine("UP");
  pumpWordCommand(transport, assembler, control, toneOutput, 1000);

  TEST_ASSERT_TRUE(toneOutput.engineRunning());
}

void test_engine_tone_flow_backward_sets_engine_running(void) {
  FakeTransport transport;
  DriveCommandAssembler assembler;
  DirectionControl control;
  FakeToneOutput toneOutput;

  transport.enqueueLine("DOWN");
  pumpWordCommand(transport, assembler, control, toneOutput, 1000);

  TEST_ASSERT_TRUE(toneOutput.engineRunning());
}

void test_engine_tone_flow_steering_alone_stays_idle(void) {
  FakeTransport transport;
  DriveCommandAssembler assembler;
  DirectionControl control;
  FakeToneOutput toneOutput;

  transport.enqueueLine("RIGHT");
  pumpWordCommand(transport, assembler, control, toneOutput, 1000);

  TEST_ASSERT_FALSE(toneOutput.engineRunning());
}

void test_engine_tone_flow_stop_returns_to_idle(void) {
  FakeTransport transport;
  DriveCommandAssembler assembler;
  DirectionControl control;
  FakeToneOutput toneOutput;

  transport.enqueueLine("UP");
  pumpWordCommand(transport, assembler, control, toneOutput, 1000);
  TEST_ASSERT_TRUE(toneOutput.engineRunning());

  transport.enqueueLine("STOP");
  pumpWordCommand(transport, assembler, control, toneOutput, 1010);
  TEST_ASSERT_FALSE(toneOutput.engineRunning());
}

void test_engine_tone_flow_disconnect_returns_to_idle(void) {
  FakeTransport transport;
  DriveCommandAssembler assembler;
  DirectionControl control;
  FakeToneOutput toneOutput;

  transport.enqueueLine("UP");
  pumpWordCommand(transport, assembler, control, toneOutput, 1000);
  TEST_ASSERT_TRUE(toneOutput.engineRunning());

  transport.setConnected(false);
  Direction safeDirection;
  if (control.onTick(false, 1010, safeDirection)) {
    toneOutput.setEngineRunning(motorEngaged(safeDirection));
  }

  TEST_ASSERT_FALSE(toneOutput.engineRunning());
}
