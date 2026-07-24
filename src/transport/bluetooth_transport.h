#pragma once

#include <BluetoothSerial.h>

#include <cstdio>
#include <string>

#include "transport/i_transport.h"

// ESP32-only. Never included by test/test_native — keeps the host test
// build free of Arduino/BluetoothSerial headers (Constitution Principle II).
class BluetoothTransport : public ITransport {
 public:
  void begin(const char* deviceName) {
    instance_ = this;
    bt_.register_callback(&BluetoothTransport::onSppEvent);
    bt_.begin(deviceName);
  }

  bool connected() override { return bt_.hasClient(); }

  std::string deviceId() override { return deviceId_; }

  bool readLine(std::string& outLine) override {
    while (bt_.available()) {
      char c = static_cast<char>(bt_.read());
      if (c == '\n') {
        outLine = buffer_;
        buffer_.clear();
        return true;
      }
      if (c != '\r') buffer_ += c;
    }
    return false;
  }

 private:
  // register_callback() takes a plain function pointer (no captured state),
  // so the single BluetoothTransport instance is tracked via a static
  // pointer set in begin().
  static void onSppEvent(esp_spp_cb_event_t event, esp_spp_cb_param_t* param) {
    if (event != ESP_SPP_SRV_OPEN_EVT || instance_ == nullptr) return;
    const uint8_t* addr = param->srv_open.rem_bda;
    char formatted[18];
    std::snprintf(formatted, sizeof(formatted), "%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1],
                  addr[2], addr[3], addr[4], addr[5]);
    instance_->deviceId_ = formatted;
  }

  static inline BluetoothTransport* instance_ = nullptr;

  BluetoothSerial bt_;
  std::string buffer_;
  std::string deviceId_;
};
