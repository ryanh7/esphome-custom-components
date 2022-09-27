#pragma once

#include <utility>

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/light/light_output.h"

namespace esphome {
namespace fpm383c {

enum Color { COLOR_GREEN = 1, COLOR_RED = 2, COLOR_YELLOW = 3, COLOR_BLUE = 4, COLOR_PURPLE = 5, COLOR_CYAN = 6 };
static const Color COLOR_MAP[6] = {COLOR_RED, COLOR_YELLOW, COLOR_GREEN, COLOR_CYAN, COLOR_BLUE, COLOR_PURPLE};

enum Status { STATUS_IDLE, STATUS_REGISTING, STATUS_MATCHING };

class TouchListener {
 public:
  virtual void on_touch(bool touched) = 0;
};

class FingerprintRegisterListener {
 public:
  virtual void on_progress(uint16_t id, uint8_t step, uint8_t progress_in_percent) = 0;
};

class FingerprintMatchListener {
 public:
  virtual void on_match(bool sucessed, uint16_t id, uint16_t score) = 0;
};

class FPM383cComponent : public uart::UARTDevice, public PollingComponent {
 public:
  void dump_config() override;
  void loop() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void set_light_level(Color color, float level);
  bool is_touched() { return this->flag_touched_; }
  void register_fingerprint();
  void clear_fingerprint();
  void cancel();
  void reset();

  void add_touch_listener(TouchListener *listener) { this->touch_listeners_.push_back(listener); }
  void add_fingerprint_register_listener(FingerprintRegisterListener *listener) {
    this->fingerprint_register_listeners_.push_back(listener);
  }
  void add_fingerprint_match_listener(FingerprintMatchListener *listener) {
    this->fingerprint_match_listeners_.push_back(listener);
  }

 protected:
  int parse_(uint8_t byte);
  uint8_t checksum_(uint8_t *data, uint32_t length);
  bool check_();
  void on_touch_(bool touched);
  void on_register_progress_(uint16_t id, uint8_t step, uint8_t progress_in_percent);
  void on_match_(bool sucessed, uint16_t id, uint16_t score);
  void command_(uint8_t cmd1, uint8_t cmd2);

  std::vector<uint8_t> rx_buffer_;
  bool flag_touched_ = false;
  uint32_t last_register_progress_time_;
  Status status_ = STATUS_IDLE;
  std::vector<TouchListener *> touch_listeners_;
  std::vector<FingerprintRegisterListener *> fingerprint_register_listeners_;
  std::vector<FingerprintMatchListener *> fingerprint_match_listeners_;
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

    // rgb to h(s)l
    int h = 0;
    float min = std::min(std::min(red, green), blue);
    float max = std::max(std::max(red, green), blue);
    float delta = max - min;

    float l = (max + min) / 2;

    if (delta == 0) {
      h = 0;
    } else {
      float hue;
      if (red == max) {
        hue = ((green - blue) / 6) / delta;
      } else if (green == max) {
        hue = (1.0f / 3) + ((blue - red) / 6) / delta;
      } else {
        hue = (2.0f / 3) + ((red - green) / 6) / delta;
      }
      if (hue < 0)
        hue += 1;
      if (hue > 1)
        hue -= 1;
      h = (int) (hue * 360);
    }

    Color color = COLOR_MAP[(h + 30) % 360 / 60];
    this->parent_->set_light_level(color, l);
  }

 protected:
  FPM383cComponent *parent_ = nullptr;
};

class TouchTrigger : public Trigger<>, public TouchListener {
 public:
  void on_touch(bool touched) override {
    if (touched) {
      this->trigger();
    }
  }
};

class ReleaseTrigger : public Trigger<>, public TouchListener {
 public:
  void on_touch(bool touched) override {
    if (!touched) {
      this->trigger();
    }
  }
};

struct RegisterProgress{
  uint16_t id;
  uint8_t step;
  uint8_t progress_in_percent;
};

class RegisterProgressTrigger : public Trigger<RegisterProgress>, public FingerprintRegisterListener {
 public:
  void on_progress(uint16_t id, uint8_t step, uint8_t progress_in_percent) override {
    RegisterProgress progress{id, step, progress_in_percent};
    this->trigger(progress);
  }
};

struct MatchResult {
  uint16_t id;
  uint16_t score;
};

class MatchSucessedTrigger : public Trigger<MatchResult>, public FingerprintMatchListener {
 public:
  void on_match(bool sucessed, uint16_t id, uint16_t score) override {
    if (sucessed) {
      MatchResult result{id, score};
      this->trigger(result);
    }
  }
};

class MatchFailedTrigger : public Trigger<>,
                           public FingerprintMatchListener {
 public:
  void on_match(bool sucessed, uint16_t id, uint16_t score) override {
    if (!sucessed) {
      this->trigger();
    }
  }
};

template<typename... Ts> class RegisterFingerprintAction : public Action<Ts...> {
 public:
  RegisterFingerprintAction(FPM383cComponent *parent) : parent_(parent) {}

  void play(Ts... x) { this->parent_->register_fingerprint(); }

 protected:
  FPM383cComponent *parent_;
};

template<typename... Ts> class ClearFingerprintAction : public Action<Ts...> {
 public:
  ClearFingerprintAction(FPM383cComponent *parent) : parent_(parent) {}

  void play(Ts... x) { this->parent_->clear_fingerprint(); }

 protected:
  FPM383cComponent *parent_;
};

template<typename... Ts> class ResetAction : public Action<Ts...> {
 public:
  ResetAction(FPM383cComponent *parent) : parent_(parent) {}

  void play(Ts... x) { this->parent_->reset(); }

 protected:
  FPM383cComponent *parent_;
};

template<typename... Ts> class CancelAction : public Action<Ts...> {
 public:
  CancelAction(FPM383cComponent *parent) : parent_(parent) {}

  void play(Ts... x) { this->parent_->cancel(); }

 protected:
  FPM383cComponent *parent_;
};



}  // namespace fpm383c
}  // namespace esphome
