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
  void setup() override;
  void dump_config() override;
  void loop() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void turn_on_light(Color color);
  void turn_off_light();
  void breathing_light(Color color, uint8_t min_level, uint8_t max_level, uint8_t rate);
  void flashing_light(Color color, uint8_t on_10ms, uint8_t off_10ms, uint8_t count);
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
  uint8_t checksum_(const uint8_t *data, const uint32_t length);
  void on_touch_(bool touched);
  void on_register_progress_(uint16_t id, uint8_t step, uint8_t progress_in_percent);
  void on_match_(bool sucessed, uint16_t id, uint16_t score);
  void command_(uint8_t cmd1, uint8_t cmd2);
  void command_(std::vector<uint8_t> &data);
  bool have_wait_();

  std::vector<uint8_t> rx_buffer_;
  bool flag_touched_ = false;
  uint32_t last_register_progress_time_, wait_at_{0};
  Status status_ = STATUS_IDLE;
  std::vector<TouchListener *> touch_listeners_;
  std::vector<FingerprintRegisterListener *> fingerprint_register_listeners_;
  std::vector<FingerprintMatchListener *> fingerprint_match_listeners_;

  char model_id_[17]{0};
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

    bool power_on;
    state->current_values_as_binary(&power_on);
    if (!power_on) {
      this->parent_->turn_off_light();
      return;
    }

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

    if (this->use_flashing_effect_) {
      this->parent_->flashing_light(color, this->on_10ms_, this->off_10ms_, this->count_);
    } else if (this->use_breathing_effect_) {
      this->parent_->breathing_light(color, this->min_level_, this->max_level_, this->rate_);
    } else {
      this->parent_->turn_on_light(color);
    }
  }

  void set_breathing_effect(bool enable, uint8_t min_level = 100, uint8_t max_level = 0, uint8_t rate = 50) {
    this->use_breathing_effect_ = enable;
    this->min_level_ = min_level;
    this->max_level_ = max_level;
    this->rate_ = rate;
  }

  void set_flashing_effect(bool enable, uint8_t on_10ms = 20, uint8_t off_10ms = 20, uint8_t count = 3) {
    this->use_flashing_effect_ = enable;
    this->on_10ms_ = on_10ms;
    this->off_10ms_ = off_10ms;
    this->count_ = count;
  }

 protected:
  FPM383cComponent *parent_ = nullptr;
  bool use_breathing_effect_{false};
  uint8_t min_level_{0}, max_level_{100}, rate_{50};
  bool use_flashing_effect_{false};
  uint8_t on_10ms_{20}, off_10ms_{20}, count_{3};
};

class BreathingLightEffect : public light::LightEffect {
 public:
  explicit BreathingLightEffect(const std::string &name) : LightEffect(name) {}

  void start() {
    FPM383cLightOutput* output = (FPM383cLightOutput *) this->state_->get_output();
    output->set_breathing_effect(true, min_brightness, max_brightness, rate);
  }

  void stop() {
    FPM383cLightOutput* output = (FPM383cLightOutput *) this->state_->get_output();
    output->set_breathing_effect(false);
  }

  void apply() override {}

  void set_min_max_rate_brightness(uint8_t min, uint8_t max, uint8_t rate) {
    this->min_brightness = min;
    this->max_brightness = max;
    this->rate = rate;
  }

 protected:
  uint8_t min_brightness{0};
  uint8_t max_brightness{100};
  uint8_t rate{50};
};

class FlashingLightEffect : public light::LightEffect {
 public:
  explicit FlashingLightEffect(const std::string &name) : LightEffect(name) {}

  void start() override {
    FPM383cLightOutput* output = (FPM383cLightOutput *) this->state_->get_output();
    output->set_flashing_effect(true, on_10ms, off_10ms, count);
  }

  void stop() override {
    FPM383cLightOutput* output = (FPM383cLightOutput *) this->state_->get_output();
    output->set_flashing_effect(false);
  }

  void apply() override {}

  void set_on_off_count_brightness(uint16_t on_ms, uint16_t off_ms, uint8_t count) {
    this->on_10ms = on_ms / 10;
    this->off_10ms = off_ms / 10;
    this->count = count;
  }

 protected:
  uint8_t on_10ms{20};
  uint8_t off_10ms{20};
  uint8_t count{3};
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
