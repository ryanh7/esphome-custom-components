#pragma once

#include "esphome/core/component.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#ifdef USE_ESP32

namespace esphome {
namespace ptx_yk1 {

class PTXYK1Device : public binary_sensor::BinarySensorInitiallyOff,
                     public esp32_ble_tracker::ESPBTDeviceListener,
                     public Component {
 public:
  void set_address(uint64_t address) { this->address_ = address; }

  void set_timeout_ms(uint32_t timeout) { this->timeout_ms_ = timeout; }

  void loop() override;

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  uint64_t address_;

  uint32_t timeout_ms_{300};
  uint32_t time_{0};
  uint32_t last_id_{0};
  bool need_timeout_{false};
};

}  // namespace ptx_yk1
}  // namespace esphome

#endif
