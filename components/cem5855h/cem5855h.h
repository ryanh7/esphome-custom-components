#pragma once

#include <utility>

#include "esphome/core/component.h"
#include "esphome/components/number/number.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace cem5855h {

class CEM5855hComponent : public uart::UARTDevice, public Component {
 public:
  void dump_config() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_moving_threshold(int threshold) { threshold_moving = threshold; };
  void set_occupancy_threshold(int threshold) { threshold_occupancy = threshold; };
  void set_moving_sensor(binary_sensor::BinarySensor *moving_sensor) { moving_sensor_ = moving_sensor; }
  void set_occupancy_sensor(binary_sensor::BinarySensor *occupancy_sensor) { occupancy_sensor_ = occupancy_sensor; }
  void set_motion_sensor(binary_sensor::BinarySensor *motion_sensor) { motion_sensor_ = motion_sensor; }

 protected:
  bool parse_(uint8_t byte);
  int threshold_moving = 250, threshold_occupancy = 250;

  std::vector<uint8_t> rx_buffer_;

  binary_sensor::BinarySensor *moving_sensor_{nullptr};
  binary_sensor::BinarySensor *occupancy_sensor_{nullptr};
  binary_sensor::BinarySensor *motion_sensor_{nullptr};
};

class ThresholdMovingNumber : public number::Number {
 public:
  void set_parent(CEM5855hComponent *parent) { this->parent_ = parent; };

 protected:
  void control(float value) override {
    if (this->parent_ != nullptr) {
      this->parent_->set_moving_threshold(value);
    }
    this->publish_state(value);
  };
  CEM5855hComponent *parent_ = nullptr;
};

class ThresholdOccupancyNumber : public number::Number {
 public:
  void set_parent(CEM5855hComponent *parent) { this->parent_ = parent; };

 protected:
  void control(float value) override {
    if (this->parent_ != nullptr) {
      this->parent_->set_occupancy_threshold(value);
    }
    this->publish_state(value);
  };
  CEM5855hComponent *parent_ = nullptr;
};

}  // namespace cem5855h
}  // namespace esphome
