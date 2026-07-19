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

}  // namespace config
