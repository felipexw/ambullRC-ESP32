#pragma once

// Centralized configuration (Constitution: no magic numbers scattered
// through the code). Extend here as new features need config values.

namespace config {

// Time without a valid command after which the connection is considered
// stale and the vehicle enters the safe state.
constexpr unsigned long kCommandTimeoutMs = 500;

constexpr int kSteerMin = -100;
constexpr int kSteerMax = 100;
constexpr int kThrottleMin = -100;
constexpr int kThrottleMax = 100;

// Steering servo (Hardware layer: PwmSteeringServo).
// This is a continuous-rotation (360°) servo, not a positional one: pulse
// width sets speed and direction, it does not hold a fixed shaft angle.
// kServoMinPulseUs/kServoMaxPulseUs are still expressed as an angle range
// (0-180) because ESP32Servo's write(angle) API requires it, but the values
// below are really "spin full speed one way" / "stop" / "spin full speed the
// other way" — kServoNeutralAngleDeg is used both at boot and as the
// fail-safe/straight-driving state, since there is no separate "resting
// position" concept on this hardware.
constexpr int kServoPin = 13;
constexpr int kServoMinAngleDeg = 0;
constexpr int kServoMaxAngleDeg = 180;
constexpr int kServoNeutralAngleDeg = 90;
constexpr int kServoMinPulseUs = 500;
constexpr int kServoMaxPulseUs = 2400;
constexpr int kServoLeftAngleDeg = 10;
constexpr int kServoRightAngleDeg = 180;
// The angle->pulse linear map (used for the LEFT/RIGHT full-speed extremes)
// does not reliably land on this servo's true stop point when applied to
// kServoNeutralAngleDeg (90 maps to 1450us, not the standard 1500us) —
// continuous-rotation servos also commonly need a small per-unit trim beyond
// even the textbook value to fully stop. So the neutral/stop state is driven
// by this pulse directly (see PwmSteeringServo), not through the angle map.
// If the servo still creeps at "stop", nudge this in ~10-20us steps.
constexpr int kServoStopPulseUs = 1500;
// Because this servo can't hold a position, a LEFT/RIGHT direction is only
// ever applied as a bounded pulse: spin toward the lock for this long, then
// MotorServoVehicleOutput::tick() automatically re-writes the stop pulse,
// regardless of whether the app is still sending LEFT/RIGHT. Without this,
// the servo spins forever the moment any turn direction is decided, since
// nothing else would ever tell it to stop. Tune to how long the physical
// steering linkage takes to swing to its lock.
constexpr unsigned long kServoTurnPulseMs = 300;

// L9110S DC motor (Hardware layer: GpioMotorDriver).
constexpr int kMotorPinA = 18;
constexpr int kMotorPinB = 19;

// Protective pause before reversing the DC motor's polarity (forward<->reverse),
// to avoid a back-EMF current spike stressing the L9110S bridge.
constexpr unsigned long kMotorReversePauseMs = 300;

}  // namespace config
