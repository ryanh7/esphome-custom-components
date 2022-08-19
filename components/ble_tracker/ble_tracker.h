#pragma once

#include "esphome/core/component.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/api/custom_api_device.h"

#ifdef USE_ESP32

namespace esphome {
namespace ble_tracker {

struct RssiData {
  std::string address;
  int address_type;
  int rssi;
};

class BLETracker : public esp32_ble_tracker::ESPBTDeviceListener, public api::CustomAPIDevice, public PollingComponent {
 public:
  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override {
    RssiData data;
    data.address = device.address_str();
    data.rssi = device.get_rssi();
    data.address_type = device.get_address_type();
    this->rssi_datas_.push_back(data);
    return false;
  }
  void dump_config() override;
  void update() override;
  void setup() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  std::vector<RssiData> rssi_datas_{};
  std::string station_;
};

}  // namespace ble_tracker
}  // namespace esphome

#endif
