#include <Arduino.h>
#include <esp_system.h>

#include "control/connection_monitor.h"
#include "control/direction_control.h"
#include "hardware/gpio_motor_driver.h"
#include "hardware/motor_servo_vehicle_output.h"
#include "hardware/pwm_steering_servo.h"
#include "hardware/serial_connection_output.h"
#include "hardware/serial_direction_output.h"
#include "protocol/command_parser.h"
#include "protocol/drive_command_assembler.h"
#include "transport/bluetooth_transport.h"

// Bluetooth Motor Control — see specs/001-bluetooth-motor-control/plan.md
// (receive/parse/decide/log) and specs/002-motor-servo-actuation/plan.md
// (the DC motor and steering servo actually move).

namespace {
BluetoothTransport transport;
DriveCommandAssembler commandAssembler;
SerialDirectionOutput output;
DirectionControl control;
ConnectionMonitor connectionMonitor;
SerialConnectionOutput connectionOutput;

GpioMotorDriver motorDriver;
PwmSteeringServo steeringServo;
MotorServoVehicleOutput hardwareOutput(motorDriver, steeringServo);

// Emits the decided direction to both the serial log and the real hardware,
// so neither can drift out of sync at a call site.
void emitDirection(Direction direction) {
  output.emit(direction);
  hardwareOutput.emit(direction);
}

// Distinguishes an intentional power cycle from an unexpected reset (e.g.
// BROWNOUT from the motor/servo current draw sagging the supply rail) —
// visible directly in the boot log instead of just inferred from symptoms.
const char* resetReasonLabel(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "POWER-ON";
    case ESP_RST_EXT: return "EXTERNAL PIN";
    case ESP_RST_SW: return "SOFTWARE";
    case ESP_RST_PANIC: return "PANIC/EXCEPTION";
    case ESP_RST_INT_WDT: return "INTERRUPT WATCHDOG";
    case ESP_RST_TASK_WDT: return "TASK WATCHDOG";
    case ESP_RST_WDT: return "OTHER WATCHDOG";
    case ESP_RST_DEEPSLEEP: return "DEEP SLEEP WAKE";
    case ESP_RST_BROWNOUT: return "BROWNOUT";
    case ESP_RST_SDIO: return "SDIO";
    default: return "UNKNOWN";
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.print("REBOOT reason=");
  Serial.println(resetReasonLabel(esp_reset_reason()));

  transport.begin("ambullrc-esp32");

  motorDriver.begin();
  steeringServo.begin();

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
    if (commandAssembler.apply(line, cmd) == ParseResult::Ok) {
      emitDirection(control.onCommand(cmd, millis()));
    }
  }

  Direction safeStateDirection;
  if (control.onTick(transport.connected(), millis(), safeStateDirection)) {
    emitDirection(safeStateDirection);
    commandAssembler.reset();
  }

  hardwareOutput.tick(millis());
}
