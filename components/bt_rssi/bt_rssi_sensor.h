#pragma once

#include "esphome/core/component.h"
#include "esphome/components/esp32_bt_tracker/esp32_bt_tracker.h"
#include "esphome/components/sensor/sensor.h"

#ifdef USE_ESP32

namespace esphome {
namespace bt_rssi {

class BTRSSISensor : public sensor::Sensor, public esp32_bt_tracker::ESPBTDeviceListener, public Component {
 public:
  void set_address(uint64_t address) {
    this->address_ = address;
  }

  void on_scan_end() override {
    if (!this->found_)
      this->publish_state(NAN);
    this->found_ = false;
  }
  
  bool parse_device(const esp32_bt_tracker::ESPBTDevice &device) override {
    if (device.address_uint64() == this->address_) {
      this->publish_state(device.get_rssi());
      this->found_ = true;
      return true;
    }
    return false;
  }
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  bool found_{false};
  uint64_t address_;
};

}  // namespace bt_rssi
}  // namespace esphome

#endif
