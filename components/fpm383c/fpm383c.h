#pragma once

#include <utility>

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/light/light_output.h"

namespace esphome {
namespace fpm383c {

enum Color { COLOR_GREEN = 1, COLOR_RED = 2, COLOR_BLUE = 4 };

class FPM383cComponent : public uart::UARTDevice, public Component {
 public:
  void dump_config() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void set_light_level(Color color, float level);

 protected:
  int parse_(uint8_t byte);
  uint8_t checksum_(uint8_t *data, uint32_t length);
  bool check_();

  std::vector<uint8_t> rx_buffer_;
  uint32_t last_byte_{0};
};

class FPM383cLightOutput : public light::LightOutput {
 public:
  void set_parent(FPM383cComponent *parent) { this->parent_ = parent; }

  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::RGB});
    return traits;
  }
  void write_state(light::LightState *state) override {
    if (this->parent_ == nullptr)
      return;
    float red, green, blue;
    state->current_values_as_rgb(&red, &green, &blue, false);
    Color color = COLOR_RED;
    float value = red;
    if (green > value) {
      color = COLOR_GREEN;
      value = green;
    }
    if (blue > value) {
      color = COLOR_BLUE;
      value = blue;
    }
    this->parent_->set_light_level(color, value);
  }

 protected:
  FPM383cComponent *parent_ = nullptr;
};

}  // namespace fpm383c
}  // namespace esphome
