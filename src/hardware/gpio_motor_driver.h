#pragma once

#include <Arduino.h>
#include <ESP32PWM.h>

#include "config.h"
#include "hardware/i_motor_driver.h"

// ESP32 real implementation: drives the L9110S bridge on
// config::kMotorPinA/kMotorPinB. Forward = A PWM at
// config::kMotorForwardDutyPercent / B LOW; reverse = A LOW / B HIGH (full
// power); stop = both LOW. Pin A is PWM'd through ESP32Servo's ESP32PWM so
// its LEDC channel/timer allocation can't collide with the steering servo's.
class GpioMotorDriver : public IMotorDriver {
 public:
  void begin() {
    pinA_.attachPin(config::kMotorPinA, config::kMotorPwmFreqHz);
    pinMode(config::kMotorPinB, OUTPUT);
    stop();
  }

  void driveForward() override {
    pinA_.writeScaled(config::kMotorForwardDutyPercent / 100.0);
    digitalWrite(config::kMotorPinB, LOW);
    Serial.println("moving DC motor to FORWARD");
  }

  void driveReverse() override {
    pinA_.writeScaled(0.0);
    digitalWrite(config::kMotorPinB, HIGH);
    Serial.println("moving DC motor to REVERSE");
  }

  void stop() override {
    pinA_.writeScaled(0.0);
    digitalWrite(config::kMotorPinB, LOW);
    Serial.println("moving DC motor to STOP");
  }

 private:
  ESP32PWM pinA_;
};
