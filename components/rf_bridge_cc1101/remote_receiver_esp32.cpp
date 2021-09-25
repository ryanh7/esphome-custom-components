#include "remote_receiver.h"
#include "esphome/core/log.h"

#ifdef ARDUINO_ARCH_ESP32
#include <driver/rmt.h>

namespace esphome {
namespace rf_bridge_cc1101 {

static const char *const TAG = "rf_bridge_cc1101.receiver.esp32";

void RemoteReceiver::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Remote Receiver...");
  this->pin_->setup();
  rmt_config_t rmt{};
  this->config_rmt(rmt);
  rmt.gpio_num = gpio_num_t(this->pin_->get_pin());
  rmt.rmt_mode = RMT_MODE_RX;
  if (this->filter_us_ == 0) {
    rmt.rx_config.filter_en = false;
  } else {
    rmt.rx_config.filter_en = true;
    rmt.rx_config.filter_ticks_thresh = this->from_microseconds(this->filter_us_);
  }
  rmt.rx_config.idle_threshold = this->from_microseconds(this->idle_us_);

  esp_err_t error = rmt_config(&rmt);
  if (error != ESP_OK) {
    this->error_code_ = error;
    this->mark_failed();
    return;
  }

  error = rmt_driver_install(this->channel_, this->buffer_size_, 0);
  if (error != ESP_OK) {
    this->error_code_ = error;
    this->mark_failed();
    return;
  }
  error = rmt_get_ringbuf_handle(this->channel_, &this->ringbuf_);
  if (error != ESP_OK) {
    this->error_code_ = error;
    this->mark_failed();
    return;
  }
}

void RemoteReceiver::enable() {
  this->pin_->setup();
  rmt_rx_start(this->channel_, true);
}

void RemoteReceiver::diable() { rmt_rx_stop(this->channel_); }

void RemoteReceiver::loop() {
  size_t len = 0;
  auto *item = (rmt_item32_t *) xRingbufferReceive(this->ringbuf_, &len, 0);
  if (item != nullptr) {
    this->decode_rmt_(item, len);
    vRingbufferReturnItem(this->ringbuf_, item);

    if (this->temp_.empty())
      return;

    this->call_listeners_dumpers_();
  }
}
void RemoteReceiver::decode_rmt_(rmt_item32_t *item, size_t len) {
  bool prev_level = false;
  uint32_t prev_length = 0;
  this->temp_.clear();
  int32_t multiplier = this->pin_->is_inverted() ? -1 : 1;

  ESP_LOGVV(TAG, "START:");
  for (size_t i = 0; i < len; i++) {
    if (item[i].level0) {
      ESP_LOGVV(TAG, "%u A: ON %uus (%u ticks)", i, this->to_microseconds(item[i].duration0), item[i].duration0);
    } else {
      ESP_LOGVV(TAG, "%u A: OFF %uus (%u ticks)", i, this->to_microseconds(item[i].duration0), item[i].duration0);
    }
    if (item[i].level1) {
      ESP_LOGVV(TAG, "%u B: ON %uus (%u ticks)", i, this->to_microseconds(item[i].duration1), item[i].duration1);
    } else {
      ESP_LOGVV(TAG, "%u B: OFF %uus (%u ticks)", i, this->to_microseconds(item[i].duration1), item[i].duration1);
    }
  }
  ESP_LOGVV(TAG, "\n");

  this->temp_.reserve(len / 4);
  for (size_t i = 0; i < len; i++) {
    if (item[i].duration0 == 0u) {
      // Do nothing
    } else if (bool(item[i].level0) == prev_level) {
      prev_length += item[i].duration0;
    } else {
      if (prev_length > 0) {
        if (prev_level) {
          this->temp_.push_back(this->to_microseconds(prev_length) * multiplier);
        } else {
          this->temp_.push_back(-int32_t(this->to_microseconds(prev_length)) * multiplier);
        }
      }
      prev_level = bool(item[i].level0);
      prev_length = item[i].duration0;
    }

    if (this->to_microseconds(prev_length) > this->idle_us_) {
      break;
    }

    if (item[i].duration1 == 0u) {
      // Do nothing
    } else if (bool(item[i].level1) == prev_level) {
      prev_length += item[i].duration1;
    } else {
      if (prev_length > 0) {
        if (prev_level) {
          this->temp_.push_back(this->to_microseconds(prev_length) * multiplier);
        } else {
          this->temp_.push_back(-int32_t(this->to_microseconds(prev_length)) * multiplier);
        }
      }
      prev_level = bool(item[i].level1);
      prev_length = item[i].duration1;
    }

    if (this->to_microseconds(prev_length) > this->idle_us_) {
      break;
    }
  }
  if (prev_length > 0) {
    if (prev_level) {
      this->temp_.push_back(this->to_microseconds(prev_length) * multiplier);
    } else {
      this->temp_.push_back(-int32_t(this->to_microseconds(prev_length)) * multiplier);
    }
  }
}

}  // namespace rf_bridge_cc1101
}  // namespace esphome

#endif
