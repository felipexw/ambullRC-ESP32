#include <Arduino.h>

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
}  // namespace

void setup() {
  Serial.begin(115200);
  transport.begin("ambullrc-esp32");
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
}
