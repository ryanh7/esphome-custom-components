#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "xiaomi_smoke_ble.h"

#ifdef USE_ESP32

namespace esphome {
namespace xiaomi_smoke_detector {

class StatusListener {
 public:
  virtual void on_change(uint8_t status) = 0;
};


class XiaomiSmokeDetector : public Component, public esp32_ble_tracker::ESPBTDeviceListener {
 public:
  void set_address(uint64_t address) { address_ = address; }
  void set_bindkey(const std::string &bindkey);

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void register_listener(StatusListener *listener) { this->listeners_.push_back(listener); }
  void set_battery_level(sensor::Sensor *battery_level) { battery_level_ = battery_level; }

 protected:
  uint64_t address_;
  std::vector<StatusListener *> listeners_;
  sensor::Sensor *battery_level_{nullptr};
  uint8_t bindkey_[16];
};

class SmokerDetectorAlarmSensor : public binary_sensor::BinarySensor, public StatusListener {
 public:
  virtual void on_change(uint8_t status) override {
    this->publish_state(status == 0x01);
  }
};

class SmokerDetectorStatusSensor : public sensor::Sensor, public StatusListener {
 public:
  virtual void on_change(uint8_t status) override {
    this->publish_state(status);
  }
};

class SmokerDetectorStatusTextSensor : public text_sensor::TextSensor, public StatusListener {
 public:
  virtual void on_change(uint8_t status) override;
};

}  // namespace xiaomi_smoke_detector
}  // namespace esphome

#endif
