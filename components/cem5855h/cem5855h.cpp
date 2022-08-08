#include "cem5855h.h"
#include "esphome/core/log.h"
#include <cstring>

namespace esphome {
namespace cem5855h {

static const char *const TAG = "cem5855h";

void CEM5855hComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "CEM5855H:");
  this->check_uart_settings(115200);
  ESP_LOGCONFIG(TAG, "  Threshold");
  ESP_LOGCONFIG(TAG, "    Moving: %d", this->threshold_moving);
  ESP_LOGCONFIG(TAG, "    Occupancy: %d", this->threshold_occupancy);
  LOG_BINARY_SENSOR("  ", "Moving", this->moving_sensor_);
  LOG_BINARY_SENSOR("  ", "Occupancy", this->occupancy_sensor_);
  LOG_BINARY_SENSOR("  ", "Motion", this->motion_sensor_);
}

void CEM5855hComponent::loop() {
  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);
    if (this->parse_(byte)) {
      ESP_LOGVV(TAG, "Parsed: 0x%02X", byte);
    } else {
      this->rx_buffer_.clear();
    }
  }
}

bool CEM5855hComponent::parse_(uint8_t byte) {
  size_t at = this->rx_buffer_.size();

  if (byte == '\r') {
    return true;
  }

  if (byte != '\n' && at < 30) {
    this->rx_buffer_.push_back(byte);
    return true;
  }

  this->rx_buffer_.push_back('\0');

  const char *raw = (const char *) &this->rx_buffer_[0];

  ESP_LOGV(TAG, "recv: %s", raw);

  bool motion = false;
  if (strncmp("mov, ", raw, 5) == 0) {
    for (int i = 5; i < at; i++) {
      if (raw[i] != ' ')
        continue;
      i++;
      int moving = atoi(&raw[i]);
      if (moving > this->threshold_moving) {
        motion = true;
        if (this->moving_sensor_ != nullptr) {
          this->moving_sensor_->publish_state(true);
          this->moving_sensor_->publish_state(false);
        }
      }
      break;
    }
  } else if (strncmp("occ, ", raw, 5) == 0) {
    for (int i = 5; i < at; i++) {
      if (raw[i] != ' ')
        continue;
      i++;
      int occupancy = atoi(&raw[i]);
      if (occupancy > this->threshold_occupancy) {
        motion = true;
        if (this->occupancy_sensor_ != nullptr) {
          this->occupancy_sensor_->publish_state(true);
          this->occupancy_sensor_->publish_state(false);
        }
      }
      break;
    }
  }

  if (motion && this->motion_sensor_ != nullptr) {
    this->motion_sensor_->publish_state(true);
    this->motion_sensor_->publish_state(false);
  }

  return false;
}

}  // namespace cem5855h
}  // namespace esphome
