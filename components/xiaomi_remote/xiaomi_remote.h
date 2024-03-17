#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "xiaomi_remote_ble.h"

#ifdef USE_ESP32

namespace esphome {
namespace xiaomi_remote {

class ButtonListener {
 public:
  virtual void on_change(uint8_t action) = 0;
  void on_change(uint16_t button, uint8_t action) {
    if (this->button_.has_value() && button != this->button_) {
      return;
    }
    this->on_change(action);
  }
  void set_button_index(uint16_t index) {this->button_ = index;}
 protected:
  optional<uint16_t> button_;
};


class XiaomiRemote : public Component, public esp32_ble_tracker::ESPBTDeviceListener {
 public:
  void set_address(uint64_t address) { address_ = address; }
  void set_bindkey(const std::string &bindkey);

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void register_listener(ButtonListener *listener) { this->listeners_.push_back(listener); }
  void set_battery_level(sensor::Sensor *battery_level) { battery_level_ = battery_level; }

 protected:
  uint64_t address_;
  std::vector<ButtonListener *> listeners_;
  sensor::Sensor *battery_level_{nullptr};
  optional<std::array<uint8_t,16>> bindkey_;
};


class RemoteBinarySensor : public binary_sensor::BinarySensor, public ButtonListener {
 public:
  virtual void on_change(uint8_t action) override {
    this->publish_state(true);
    this->publish_state(false);
  }
};

class RemoteSensor : public sensor::Sensor, public ButtonListener {
 public:
  virtual void on_change(uint8_t action) override;
};

class RemoteTextSensor : public text_sensor::TextSensor, public ButtonListener {
 public:
  virtual void on_change(uint8_t action) override;
};

class ClickTrigger : public Trigger<>, public ButtonListener {
 public:
  void on_change(uint8_t action) override {
    if (action == BUTTON_TYPE_CLICK) {
      this->trigger();
    }
  }
};

class DoubleClickTrigger : public Trigger<>, public ButtonListener {
 public:
  void on_change(uint8_t action) override {
    if (action == BUTTON_TYPE_DOUBLE_CLICK) {
      this->trigger();
    }
  }
};

class TripleClickTrigger : public Trigger<>, public ButtonListener {
 public:
  void on_change(uint8_t action) override {
    if (action == BUTTON_TYPE_TRIPLE_CLICK) {
      this->trigger();
    }
  }
};

class LongPressTrigger : public Trigger<>, public ButtonListener {
 public:
  void on_change(uint8_t action) override {
    if (action == BUTTON_TYPE_LONG_PRESS) {
      this->trigger();
    }
  }
};

}  // namespace xiaomi_remote
}  // namespace esphome

#endif
