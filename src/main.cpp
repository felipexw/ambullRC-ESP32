#include <Arduino.h>
#include <ESP32Servo.h>

#include "config.h"
#include "control/connection_monitor.h"
#include "control/direction_control.h"
#include "hardware/serial_connection_output.h"
#include "hardware/serial_direction_output.h"
#include "protocol/command_parser.h"
#include "transport/bluetooth_transport.h"

// Bluetooth Motor Control (direction-print slice) — see
// specs/001-bluetooth-motor-control/plan.md. Receives drive commands over
// Bluetooth and prints the decided direction. No servo/DC-motor output yet.

namespace {
BluetoothTransport transport;
SerialDirectionOutput output;
DirectionControl control;
ConnectionMonitor connectionMonitor;
SerialConnectionOutput connectionOutput;

// --- SG90 hardware bring-up test ---
// Standalone wiring/sanity check for the steering servo. Deliberately not
// wired through the Protocol/Control layers yet (no real feature here) — it
// only exists to confirm the SG90 is wired correctly and moves as expected.
Servo steeringServo;
int servoAngleDeg = config::kServoNeutralAngleDeg;
int servoStepDeg = 1;
unsigned long lastServoStepAtMs = 0;
constexpr unsigned long kServoStepIntervalMs = 15;

// Clamps to the SG90's safe range and moves it to the given angle.
void moveServoTo(int angleDeg) {
  angleDeg = constrain(angleDeg, config::kServoMinAngleDeg, config::kServoMaxAngleDeg);
  steeringServo.write(angleDeg);
}

// Sweeps the servo end-to-end, one step per interval, so the motion is
// visible to a human confirming the wiring — not a real control loop.
void stepServoSweep(unsigned long nowMs) {
  if (nowMs - lastServoStepAtMs < kServoStepIntervalMs) return;
  lastServoStepAtMs = nowMs;

  servoAngleDeg += servoStepDeg;
  if (servoAngleDeg >= config::kServoMaxAngleDeg || servoAngleDeg <= config::kServoMinAngleDeg) {
    servoStepDeg = -servoStepDeg;
  }
  moveServoTo(servoAngleDeg);
}
}  // namespace

void setup() {
  Serial.begin(115200);
  transport.begin("ambullrc-esp32");

  steeringServo.setPeriodHertz(50);
  steeringServo.attach(config::kServoPin, config::kServoMinPulseUs, config::kServoMaxPulseUs);
  moveServoTo(config::kServoNeutralAngleDeg);

  Serial.println("READY: ambullrc-esp32");
}

void loop() {
  ConnectionEvent connectionEvent = connectionMonitor.onTick(transport.connected());
  if (connectionEvent != ConnectionEvent::None) {
    connectionOutput.emit(connectionEvent, transport.deviceId());
  }

  std::string line;
  if (transport.readLine(line)) {
    DriveCommand cmd;
    if (parseLine(line, cmd) == ParseResult::Ok) {
      output.emit(control.onCommand(cmd, millis()));
    }
  }

  Direction safeStateDirection;
  if (control.onTick(transport.connected(), millis(), safeStateDirection)) {
    output.emit(safeStateDirection);
  }

  stepServoSweep(millis());
}
