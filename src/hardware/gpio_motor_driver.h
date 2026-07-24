#pragma once

#include <Arduino.h>

#include "config.h"
#include "hardware/i_motor_driver.h"

// ESP32 real implementation: drives the L9110S bridge via digitalWrite on
// config::kMotorPinA/kMotorPinB. Forward = A HIGH / B LOW; reverse = A LOW /
// B HIGH; stop = both LOW.
class GpioMotorDriver : public IMotorDriver {
 public:
  void begin() {
    pinMode(config::kMotorPinA, OUTPUT);
    pinMode(config::kMotorPinB, OUTPUT);
    stop();
  }

  void driveForward() override {
    digitalWrite(config::kMotorPinA, HIGH);
    digitalWrite(config::kMotorPinB, LOW);
    Serial.println("moving DC motor to FORWARD");
  }

  void driveReverse() override {
    digitalWrite(config::kMotorPinA, LOW);
    digitalWrite(config::kMotorPinB, HIGH);
    Serial.println("moving DC motor to REVERSE");
  }

  void stop() override {
    digitalWrite(config::kMotorPinA, LOW);
    digitalWrite(config::kMotorPinB, LOW);
    Serial.println("moving DC motor to STOP");
  }
};
