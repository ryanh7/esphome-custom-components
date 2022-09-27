#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "xiaomi_toothbrush_ble.h"

#ifdef USE_ESP32

namespace esphome {
namespace xiaomi_m1st500 {

class XiaomiM1ST500 : public Component, public esp32_ble_tracker::ESPBTDeviceListener {
 public:
  void set_address(uint64_t address) { address_ = address; }

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void set_score(sensor::Sensor *score) { score_ = score; }
  void set_battery_level(sensor::Sensor *battery_level) { battery_level_ = battery_level; }

 protected:
  uint64_t address_;
  sensor::Sensor *score_{nullptr};
  sensor::Sensor *battery_level_{nullptr};
};

}  // namespace xiaomi_m1st500
}  // namespace esphome

#endif
