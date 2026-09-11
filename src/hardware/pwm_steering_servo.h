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
    // Positional 180° micro servo: it holds whatever angle it's told, so it
    // doesn't need a continuous "stop" pulse driven at boot — but it also
    // doesn't know where it physically is after a reset, so it stays put at
    // whatever angle it was left at until the first real command arrives.
    // Explicitly recenter it here so every boot starts from a known state.
    setAngleDeg(config::kServoNeutralAngleDeg);
  }

  void setAngleDeg(int angleDeg) override {
    angleDeg = constrain(angleDeg, config::kServoMinAngleDeg, config::kServoMaxAngleDeg);
    // Skip re-issuing the same angle: the servo is already there, and a
    // redundant write is, at best, wasted PWM churn — see i_steering_servo.h.
    if (hasWritten_ && angleDeg == lastAngleDeg_) return;
    hasWritten_ = true;
    lastAngleDeg_ = angleDeg;

    Serial.print("servo angle: ");
    Serial.println(angleDeg);
    // The stop state is driven by an explicit calibrated pulse rather than
    // through the angle->pulse map — see kServoStopPulseUs in config.h.
    if (angleDeg == config::kServoNeutralAngleDeg) {
      servo_.writeMicroseconds(config::kServoStopPulseUs);
    } else {
      servo_.write(angleDeg);
    }
  }

 private:
  Servo servo_;
  bool hasWritten_ = false;
  int lastAngleDeg_ = 0;
};
