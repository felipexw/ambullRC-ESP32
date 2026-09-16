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
// PwmSteeringServo::setAngleDeg().
constexpr int kServoMinAngleDeg = 0;
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

// Onboard sound effects (Hardware layer: PwmToneOutput). A single GPIO
// driving a speaker/buzzer via the ESP32's LEDC tone-generation peripheral
// (the same PWM hardware family PwmSteeringServo uses) — not the built-in
// analog DAC (fixed to GPIO25/26, which would collide with kLight4Pin) and
// not an external I2S DAC/amp. Volume is fixed by a hardware trim
// potentiometer downstream of this pin — there is no software volume
// control.
constexpr int kToneOutputPin = 26;
// The installed Arduino-ESP32 core (2.x-generation LEDC API, per
// framework-arduinoespressif32 @ 3.20017.241212) addresses LEDC by channel
// number, not pin, separately from ledcAttachPin(). Channel 15 (the last of
// 16) is used to stay clear of whatever low channel ESP32Servo auto-allocates
// for the steering servo.
constexpr int kToneLedcChannel = 15;

// Horn: fixed duration and tone per explicit spec (FR-002/FR-003a). A
// one-shot trigger, unlike the engine tone below.
constexpr unsigned long kHornDurationMs = 1500;
constexpr int kHornFreqHz = 420;

// Engine: no longer a triggered one-shot effect — it plays continuously,
// automatically reflecting the DC motor's current state
// (IToneOutput::setEngineRunning), so there is no duration constant.
//
// Idle "V8 burble": firing frequency (Hz) = (RPM / 60) x (cylinders / 2); a
// V8 at a Mustang-like idle of ~775 RPM fires at (775/60) x 4 ~= 52 Hz
// (research.md §6) — the wobble around this base is what PwmToneOutput uses
// to approximate the characteristic lopey idle. Plays whenever the DC motor
// isn't engaged.
constexpr int kEngineIdleBaseFreqHz = 52;
constexpr int kEngineIdleWobbleFreqHz = 6;

// "Running": a higher, rougher firing frequency approximating a
// light-throttle cruise (~2200 RPM -> (2200/60) x 4 ~= 147 Hz), deliberately
// distinct from the idle rumble above. Plays whenever the DC motor is
// engaged (UP or DOWN), regardless of direction or steering.
constexpr int kEngineRunningBaseFreqHz = 147;
constexpr int kEngineRunningWobbleFreqHz = 15;

// Horn-over-engine layering: a single LEDC channel can only output one
// frequency at a time, so the horn is made to sound "on top of" the engine
// by rapidly time-slicing between the horn tone and whichever engine tone is
// currently playing, within each short window below, rather than silencing
// the engine while the horn plays.
constexpr unsigned long kToneLayerPeriodMs = 100;
constexpr unsigned long kToneLayerHornSliceMs = 60;

}  // namespace config
