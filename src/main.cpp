#include <Arduino.h>

// Toolchain smoke test: confirms build/flash pipeline works end-to-end.
// Replace with real Transport/Protocol/Control/Hardware layers per AGENTS.md.

void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.println("ambullrc-esp32 alive");
  delay(500);
}
