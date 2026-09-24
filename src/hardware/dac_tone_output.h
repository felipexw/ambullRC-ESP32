#pragma once

#include <Arduino.h>
#include <esp_timer.h>

#include "config.h"
#include "hardware/horn_waveform.h"
#include "hardware/i_tone_output.h"

// ESP32 real implementation: plays a synthesized horn tone by writing
// directly to the built-in 8-bit DAC on config::kToneOutputPin (GPIO26),
// toggling between two levels at the horn frequency for a fixed duration.
// No I2S, no DMA, no stored sample.
//
// The waveform is clocked by a periodic esp_timer (one tick per
// half-period, running on the ESP-IDF timer task) rather than a
// delayMicroseconds() loop, so playHorn() returns immediately: drive and
// steering commands keep flowing through loop() while the horn plays
// (spec 007 FR-007). hornBusy() is true until the tone has finished, so a
// retrigger while playing is ignored (FR-004).
class DacToneOutput : public IToneOutput {
 public:
  void begin() {
    dacWrite(config::kToneOutputPin, HornWaveform::kMidline);
    lastLevel_ = HornWaveform::kMidline;

    // Runs for the life of the program: while idle each tick just returns
    // the midline, which isn't rewritten. Never stopping/restarting the
    // timer avoids any start/stop race with the tick in flight.
    esp_timer_create_args_t args = {};
    args.callback = &DacToneOutput::onHalfPeriod;
    args.arg = this;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "horn";
    esp_timer_handle_t timer;
    ESP_ERROR_CHECK(esp_timer_create(&args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, HornWaveform::kHalfPeriodUs));
  }

  void playHorn() override { wave_.start(); }

  bool hornBusy() override { return wave_.busy(); }

  void tick(unsigned long /*nowMs*/) override {}

 private:
  static void onHalfPeriod(void* arg) {
    DacToneOutput* self = static_cast<DacToneOutput*>(arg);
    uint8_t level = self->wave_.step();
    if (level == self->lastLevel_) return;
    dacWrite(config::kToneOutputPin, level);
    self->lastLevel_ = level;
  }

  HornWaveform wave_;
  uint8_t lastLevel_ = HornWaveform::kMidline;  // only touched on the timer task after begin()
};
