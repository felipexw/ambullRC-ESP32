#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>

#include "config.h"
#include "hardware/i_steering_servo.h"

// ESP32 real implementation: wraps the ESP32Servo library's Servo class.
class PwmSteeringServo : public ISteeringServo {
 public:
  void begin() {
    servo_.setPeriodHertz(50);
    servo_.attach(config::kServoPin, config::kServoMinPulseUs, config::kServoMaxPulseUs);
    // This is a continuous-rotation (360°) servo: pulse width controls speed
    // and direction, not a held position. Boot must land on the stop pulse
    // (kServoNeutralAngleDeg), same as the fail-safe state — any other value
    // (e.g. a positional-servo-style "0 degrees") spins the shaft at speed
    // with nothing to stop it until the first command arrives.
    setAngleDeg(config::kServoNeutralAngleDeg);
  }

  void setAngleDeg(int angleDeg) override {
    angleDeg = constrain(angleDeg, config::kServoMinAngleDeg, config::kServoMaxAngleDeg);
    if (!hasWritten_ || angleDeg != lastAngleDeg_) {
      Serial.print("moving servo motor to ");
      Serial.print(labelFor(angleDeg));
      Serial.print(" (");
      Serial.print(angleDeg);
      Serial.println(" deg)");
      hasWritten_ = true;
      lastAngleDeg_ = angleDeg;
    }
    // The stop state is driven by an explicit calibrated pulse rather than
    // through the angle->pulse map — see kServoStopPulseUs in config.h.
    if (angleDeg == config::kServoNeutralAngleDeg) {
      servo_.writeMicroseconds(config::kServoStopPulseUs);
    } else {
      servo_.write(angleDeg);
    }
  }

 private:
  static const char* labelFor(int angleDeg) {
    if (angleDeg == config::kServoLeftAngleDeg) return "LEFT";
    if (angleDeg == config::kServoRightAngleDeg) return "RIGHT";
    if (angleDeg == config::kServoNeutralAngleDeg) return "STOP";
    return "UNKNOWN";
  }

  Servo servo_;
  bool hasWritten_ = false;
  int lastAngleDeg_ = 0;
};
