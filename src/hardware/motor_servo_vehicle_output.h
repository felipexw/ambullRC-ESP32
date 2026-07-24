#pragma once

#include "config.h"
#include "control/direction.h"
#include "hardware/i_motor_driver.h"
#include "hardware/i_steering_servo.h"
#include "hardware/i_vehicle_output.h"

// Hardware layer implementation of IVehicleOutput that actually drives the
// DC motor and steering servo from a decided Direction, per
// specs/002-motor-servo-actuation/contracts/direction-to-actuation-contract.md.
//
// The steering servo is continuous-rotation (360°), not positional: it has
// no way to hold a fixed angle, only a speed/direction set by the commanded
// pulse. So a LEFT/RIGHT direction is applied as a bounded pulse
// (config::kServoTurnPulseMs) in tick() — long enough to swing the steering
// linkage to its lock — after which tick() automatically re-writes the
// neutral/stop pulse, regardless of whether the Direction is still
// LEFT/RIGHT. The motor's polarity is applied immediately in tick() UNLESS
// the change is a true reversal (Forward<->Reverse with neither side
// Stopped), in which case the motor is stopped immediately and a protective
// pause (config::kMotorReversePauseMs) is observed before engaging the new
// polarity — see data-model.md's state machine. Going to/from Stopped is
// never subject to the pause, so a fail-safe STOP always stops immediately.
class MotorServoVehicleOutput : public IVehicleOutput {
 public:
  enum class MotorPolarity { Stopped, Forward, Reverse };

  MotorServoVehicleOutput(IMotorDriver& motor, ISteeringServo& servo)
      : motor_(motor), servo_(servo) {}

  void emit(Direction direction) override {
    desiredPolarity_ = polarityFor(direction);
    desiredServoAngle_ = servoAngleFor(direction);
  }

  void tick(unsigned long nowMs) override {
    tickServo(nowMs);
    tickMotor(nowMs);
  }

 private:
  void tickServo(unsigned long nowMs) {
    if (desiredServoAngle_ != appliedServoAngle_) {
      servo_.setAngleDeg(desiredServoAngle_);
      appliedServoAngle_ = desiredServoAngle_;
      steeringPulseActive_ = appliedServoAngle_ != config::kServoNeutralAngleDeg;
      steerPulseStartedAtMs_ = nowMs;
      return;
    }
    if (steeringPulseActive_ && nowMs - steerPulseStartedAtMs_ >= config::kServoTurnPulseMs) {
      servo_.setAngleDeg(config::kServoNeutralAngleDeg);
      appliedServoAngle_ = config::kServoNeutralAngleDeg;
      steeringPulseActive_ = false;
    }
  }

  void tickMotor(unsigned long nowMs) {
    if (pausing_) {
      if (nowMs - pauseStartedAtMs_ >= config::kMotorReversePauseMs) {
        pausing_ = false;
        applyPolarity(desiredPolarity_);
      }
      return;
    }

    if (desiredPolarity_ == appliedPolarity_) return;

    const bool isTrueReversal = appliedPolarity_ != MotorPolarity::Stopped &&
                                 desiredPolarity_ != MotorPolarity::Stopped;
    if (isTrueReversal) {
      motor_.stop();
      appliedPolarity_ = MotorPolarity::Stopped;
      pausing_ = true;
      pauseStartedAtMs_ = nowMs;
    } else {
      applyPolarity(desiredPolarity_);
    }
  }

  static MotorPolarity polarityFor(Direction direction) {
    switch (direction) {
      case Direction::Forward:
      case Direction::ForwardLeft:
      case Direction::ForwardRight:
        return MotorPolarity::Forward;
      case Direction::Backward:
      case Direction::BackwardLeft:
      case Direction::BackwardRight:
        return MotorPolarity::Reverse;
      default:
        return MotorPolarity::Stopped;
    }
  }

  static int servoAngleFor(Direction direction) {
    switch (direction) {
      case Direction::Left:
      case Direction::ForwardLeft:
      case Direction::BackwardLeft:
        return config::kServoLeftAngleDeg;
      case Direction::Right:
      case Direction::ForwardRight:
      case Direction::BackwardRight:
        return config::kServoRightAngleDeg;
      default:
        return config::kServoNeutralAngleDeg;
    }
  }

  void applyPolarity(MotorPolarity polarity) {
    switch (polarity) {
      case MotorPolarity::Forward:
        motor_.driveForward();
        break;
      case MotorPolarity::Reverse:
        motor_.driveReverse();
        break;
      case MotorPolarity::Stopped:
        motor_.stop();
        break;
    }
    appliedPolarity_ = polarity;
  }

  IMotorDriver& motor_;
  ISteeringServo& servo_;
  MotorPolarity desiredPolarity_ = MotorPolarity::Stopped;
  MotorPolarity appliedPolarity_ = MotorPolarity::Stopped;
  bool pausing_ = false;
  unsigned long pauseStartedAtMs_ = 0;

  int desiredServoAngle_ = config::kServoNeutralAngleDeg;
  int appliedServoAngle_ = config::kServoNeutralAngleDeg;
  bool steeringPulseActive_ = false;
  unsigned long steerPulseStartedAtMs_ = 0;
};
