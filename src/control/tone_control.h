#pragma once

// Pure decision logic: honors a horn trigger only when the horn isn't
// already playing — a retrigger while playing is ignored (FR-004); the user
// has to wait for it to finish and press again.
class ToneControl {
 public:
  bool apply(bool hornBusy);
};
