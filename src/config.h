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
// This is a positional 180° micro servo (90g SG90-class): a commanded angle
// is held at that shaft position, unlike the continuous-rotation unit this
// rig originally used. kServoNeutralAngleDeg is the centered/straight
// position, used both at boot and as the fail-safe/straight-driving state.
constexpr int kServoPin = 13;
// Mechanical lock-to-lock range for this test rig: the linkage can't safely
// travel past these without binding, so this is also the hard clamp applied
// in PwmSteeringServo::setAngleDeg().
constexpr int kServoMinAngleDeg = 5;
constexpr int kServoMaxAngleDeg = 180;
constexpr int kServoNeutralAngleDeg = 90;
constexpr int kServoMinPulseUs = 500;
constexpr int kServoMaxPulseUs = 2400;
constexpr int kServoLeftAngleDeg = kServoMinAngleDeg;
constexpr int kServoRightAngleDeg = kServoMaxAngleDeg;
// The angle->pulse linear map (500-2400us across 0-180°) doesn't land
// exactly on the standard 1500us center pulse at kServoNeutralAngleDeg (90
// maps to ~1450us), nudging the wheels slightly off true center. So the
// neutral/straight state is driven by this explicit calibrated pulse
// instead (see PwmSteeringServo), guaranteeing a precisely centered angle.
// If the wheels still sit slightly off-center at neutral, nudge this in
// ~10-20us steps.
constexpr int kServoStopPulseUs = 1500;
// LEFT/RIGHT taps swing the servo to its full lock and hold it there only
// for this long before MotorServoVehicleOutput automatically re-centers it,
// regardless of whether the app is still sending LEFT/RIGHT — a momentary
// tap-to-turn behavior that also avoids stalling the servo against its
// mechanical end-stop indefinitely. Tune to how long the physical steering
// linkage takes to swing to its lock.
constexpr unsigned long kServoTurnPulseMs = 300;

// L9110S DC motor (Hardware layer: GpioMotorDriver).
constexpr int kMotorPinA = 18;
constexpr int kMotorPinB = 19;

// Protective pause before reversing the DC motor's polarity (forward<->reverse),
// to avoid a back-EMF current spike stressing the L9110S bridge.
constexpr unsigned long kMotorReversePauseMs = 300;

// Connection status LED (Hardware layer: LedConnectionOutput). Not a
// strapping pin (avoids GPIO0/2/5/12/15), so it can't interfere with boot
// mode selection.
constexpr int kLedPin = 12;

}  // namespace config
