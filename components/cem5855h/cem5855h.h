#pragma once

#include <utility>

#include "esphome/core/component.h"
#include "esphome/components/number/number.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace cem5855h {

class ThresholdSensor : public binary_sensor::BinarySensor {
 public:
  void set_moving_threshold(int moving) { this->moving_threshold_ = moving; }
  int get_moving_threshold() { return this->moving_threshold_; }
  void set_occupancy_threshold(int occupancy) { this->occupancy_threshold_ = occupancy; }
  int get_occupancy_threshold() { return this->occupancy_threshold_; }
  void update_moving(int value) {
    if (value >= this->moving_threshold_) {
      this->publish_state(true);
      this->publish_state(false);
    }
  }
  void update_occupancy(int value) {
    if (value >= this->occupancy_threshold_) {
      this->publish_state(true);
      this->publish_state(false);
    }
  }

 protected:
  int moving_threshold_{250}, occupancy_threshold_{250};
};

class CEM5855hComponent : public uart::UARTDevice, public Component {
 public:
  void dump_config() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void register_moving_sensor(ThresholdSensor *moving_sensor) { moving_sensors_.push_back(moving_sensor); }
  void register_occupancy_sensor(ThresholdSensor *occupancy_sensor) { occupancy_sensors_.push_back(occupancy_sensor); }
  void register_motion_sensor(ThresholdSensor *motion_sensor) { motion_sensors_.push_back(motion_sensor); }

 protected:
  bool parse_(uint8_t byte);

  std::vector<uint8_t> rx_buffer_;

  std::vector<ThresholdSensor *> moving_sensors_;
  std::vector<ThresholdSensor *> occupancy_sensors_;
  std::vector<ThresholdSensor *> motion_sensors_;
};

class ThresholdMovingNumber : public number::Number {
 public:
  void set_parent(ThresholdSensor *parent) { this->parent_ = parent; };

 protected:
  void control(float value) override {
    if (this->parent_ != nullptr) {
      this->parent_->set_moving_threshold(value);
    }
    this->publish_state(value);
  };
  ThresholdSensor *parent_{nullptr};
};

class ThresholdOccupancyNumber : public number::Number {
 public:
  void set_parent(ThresholdSensor *parent) { this->parent_ = parent; };

 protected:
  void control(float value) override {
    if (this->parent_ != nullptr) {
      this->parent_->set_occupancy_threshold(value);
    }
    this->publish_state(value);
  };
  ThresholdSensor *parent_{nullptr};
};

}  // namespace cem5855h
}  // namespace esphome
