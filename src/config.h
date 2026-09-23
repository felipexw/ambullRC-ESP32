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
// Lock-to-lock range for this test rig, also the hard clamp applied in
// PwmSteeringServo::setAngleDeg(). Recalibrated on-device: the servo horn's
// mounting offset against the steering linkage means the wheel-straight
// point isn't the middle of this range (60 sits closer to the left lock at
// 30 than to the right lock at 160) — confirmed by driving the car, not a
// typo.
constexpr int kServoMinAngleDeg = 30;
constexpr int kServoMaxAngleDeg = 160;
constexpr int kServoNeutralAngleDeg = 60;
constexpr int kServoMinPulseUs = 500;
constexpr int kServoMaxPulseUs = 2400;
constexpr int kServoLeftAngleDeg = kServoMinAngleDeg;
constexpr int kServoRightAngleDeg = kServoMaxAngleDeg;
// The angle->pulse linear map (500-2400us across 0-180°, the Servo library's
// own write() convention — independent of this rig's kServoMinAngleDeg/
// kServoMaxAngleDeg lock-to-lock limits above) doesn't land exactly on the
// standard 1500us center pulse at kServoNeutralAngleDeg (60 maps to
// ~1133us). So the neutral/straight state is driven by this explicit
// calibrated pulse instead (see PwmSteeringServo), guaranteeing a precisely
// centered angle. If the wheels still sit slightly off-center at neutral,
// nudge this in ~10-20us steps.
constexpr int kServoStopPulseUs = 1500;
// LEFT/RIGHT taps swing the servo to its full lock and hold it there only
// for this long before MotorServoVehicleOutput automatically re-centers it,
// regardless of whether the app is still sending LEFT/RIGHT — a momentary
// tap-to-turn behavior that also avoids stalling the servo against its
// mechanical end-stop indefinitely. Tune to how long the physical steering
// linkage takes to swing to its lock.
constexpr unsigned long kServoTurnPulseMs = 150;

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

// Auxiliary lights (Hardware layer: GpioLightsOutput). Four independent
// GPIOs, none a strapping pin (avoids GPIO0/2/5/12/15) or input-only
// (avoids GPIO34-39), so none can interfere with boot mode selection.
constexpr int kLightCount = 4;
constexpr int kLight1Pin = 21;
constexpr int kLight2Pin = 22;
constexpr int kLight3Pin = 23;
constexpr int kLight4Pin = 25;

// Onboard sound effects (Hardware layer: DacToneOutput). A single GPIO
// driving a speaker/amp module directly via the ESP32's built-in 8-bit DAC
// (DAC channel 2). Only DAC channel 2 is enabled: GPIO25 (DAC channel 1) is
// kLight4Pin and must not be claimed. Volume is fixed by a hardware trim
// potentiometer downstream of this pin — there is no software volume
// control.
constexpr int kToneOutputPin = 26;

// Horn: a synthesized square-wave tone written directly to the DAC
// (DacToneOutput::playHorn() blocks for the full duration).
constexpr unsigned long kHornDurationMs = 1500;
constexpr int kHornFreqHz = 420;

}  // namespace config
