#include <Arduino.h>
#include <esp_system.h>

#include "control/connection_monitor.h"
#include "control/direction_control.h"
#include "control/lights_control.h"
#include "control/tone_control.h"
#include "hardware/gpio_lights_output.h"
#include "hardware/gpio_motor_driver.h"
#include "hardware/led_connection_output.h"
#include "hardware/motor_servo_vehicle_output.h"
#include "hardware/pwm_steering_servo.h"
#include "hardware/pwm_tone_output.h"
#include "hardware/serial_connection_output.h"
#include "hardware/serial_direction_output.h"
#include "protocol/command_parser.h"
#include "protocol/drive_command_assembler.h"
#include "protocol/light_command_parser.h"
#include "protocol/lights_report_formatter.h"
#include "protocol/tone_command_parser.h"
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
LedConnectionOutput ledOutput;

GpioMotorDriver motorDriver;
PwmSteeringServo steeringServo;
MotorServoVehicleOutput hardwareOutput(motorDriver, steeringServo);

LightsControl lightsControl;
GpioLightsOutput lightsOutput;

ToneControl toneControl;
PwmToneOutput toneOutput;

// Emits the decided direction to both the serial log and the real hardware,
// so neither can drift out of sync at a call site.
void emitDirection(Direction direction) {
  output.emit(direction);
  hardwareOutput.emit(direction);
}

// Sends the lights state report to the app over Bluetooth, logging exactly
// what was sent and when — the two writeLine() call sites below (on a real
// light change, and on a fresh connection) both go through here so neither
// can log something different from what actually went out.
void sendLightsReport(const LightsState& state) {
  std::string report = formatLightsReport(state);
  Serial.print("sent to app: ");
  Serial.println(report.c_str());
  transport.writeLine(report);
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
  ledOutput.begin();
  lightsOutput.begin();
  toneOutput.begin();

  Serial.println("READY: ambullrc-esp32");
}

void loop() {
  ConnectionEvent connectionEvent = connectionMonitor.onTick(transport.connected());
  if (connectionEvent != ConnectionEvent::None) {
    connectionOutput.emit(connectionEvent, transport.deviceId());
    ledOutput.emit(connectionEvent, transport.deviceId());
    if (connectionEvent == ConnectionEvent::Connected) {
      sendLightsReport(lightsControl.state());
    }
  }

  std::string line;
  if (transport.readLine(line)) {
    LightCommand lightCmd;
    if (parseLightCommand(line, lightCmd) == ParseResult::Ok) {
      LightsState lightsState;
      bool lightsChanged = lightsControl.apply(lightCmd, lightsState);
      lightsOutput.apply(lightsState);

      Serial.print("received: ");
      Serial.println(line.c_str());
      Serial.println(" -> LIGHT1: ");
      Serial.print(lightsState[0] ? "ON" : "OFF");
      Serial.print(", LIGHT2: ");
      Serial.print(lightsState[1] ? "ON" : "OFF");
      Serial.print(", LIGHT3: ");
      Serial.print(lightsState[2] ? "ON" : "OFF");
      Serial.print(", LIGHT4: ");
      Serial.println(lightsState[3] ? "ON" : "OFF");

      if (lightsChanged) sendLightsReport(lightsState);
    } else {
      Serial.print("received: ");
      Serial.println(line.c_str());

      if (parseHornCommand(line) == ParseResult::Ok) {
        if (toneControl.apply(toneOutput.hornBusy())) {
          toneOutput.playHorn();
        }
      } else {
        DriveCommand cmd;
        if (commandAssembler.apply(line, cmd) == ParseResult::Ok) {
          if (cmd.steer < 0) {
            Serial.println("steer command received: LEFT");
          } else if (cmd.steer > 0) {
            Serial.println("steer command received: RIGHT");
          }
          Direction direction = control.onCommand(cmd, millis());
          emitDirection(direction);
          toneOutput.setEngineRunning(motorEngaged(direction));
        }
      }
    }
  }

  Direction safeStateDirection;
  if (control.onTick(transport.connected(), millis(), safeStateDirection)) {
    emitDirection(safeStateDirection);
    commandAssembler.reset();
    toneOutput.setEngineRunning(motorEngaged(safeStateDirection));
  }

  hardwareOutput.tick(millis());
  toneOutput.tick(millis());
}
