#pragma once

#include "esphome/core/automation.h"
#include "esp32_bt_tracker.h"

#ifdef USE_ESP32

namespace esphome {
namespace esp32_bt_tracker {
class ESPBTAdvertiseTrigger : public Trigger<const ESPBTDevice &>, public ESPBTDeviceListener {
 public:
  explicit ESPBTAdvertiseTrigger(ESP32BTTracker *parent) { parent->register_listener(this); }
  void set_address(uint64_t address) { this->address_ = address; }

  bool parse_device(const ESPBTDevice &device) override {
    if (this->address_ && device.address_uint64() != this->address_) {
      return false;
    }
    this->trigger(device);
    return true;
  }

 protected:
  uint64_t address_ = 0;
};



}  // namespace esp32_bt_tracker
}  // namespace esphome

#endif
