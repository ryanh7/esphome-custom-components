#include "cem5855h.h"
#include "esphome/core/log.h"
#include <cstring>

namespace esphome {
namespace cem5855h {

static const char *const TAG = "cem5855h";

void CEM5855hComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "CEM5855H:");
  this->check_uart_settings(115200);
  if (!this->moving_sensors_.empty()) {
    ESP_LOGCONFIG(TAG, "  Moving Sensors:");
    for (auto *sensor : this->moving_sensors_) {
      LOG_BINARY_SENSOR("  ", "Moving", sensor);
      ESP_LOGCONFIG(TAG, "    Threshold: %d", sensor->get_moving_threshold());
    }
  }
  if (!this->occupancy_sensors_.empty()) {
    ESP_LOGCONFIG(TAG, "  Occupancy Sensors:");
    for (auto *sensor : this->occupancy_sensors_) {
      LOG_BINARY_SENSOR("  ", "Occupancy", sensor);
      ESP_LOGCONFIG(TAG, "    Threshold: %d", sensor->get_occupancy_threshold());
    }
  }
  if (!this->motion_sensors_.empty()) {
    ESP_LOGCONFIG(TAG, "  Motion Sensors:");
    for (auto *sensor : this->motion_sensors_) {
      LOG_BINARY_SENSOR("  ", "Motion", sensor);
      ESP_LOGCONFIG(TAG, "    Threshold:");
      ESP_LOGCONFIG(TAG, "      Moving: %d", sensor->get_moving_threshold());
      ESP_LOGCONFIG(TAG, "      Occupancy: %d", sensor->get_occupancy_threshold());
    }
  }
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

  if (strncmp("mov, ", raw, 5) == 0) {
    for (int i = 5; i < at; i++) {
      if (raw[i] != ' ')
        continue;
      i++;
      int moving = atoi(&raw[i]);
      for (auto *sensor : this->moving_sensors_) {
        sensor->update_moving(moving);
      }
      for (auto *sensor : this->motion_sensors_) {
        sensor->update_moving(moving);
      }
      break;
    }
  } else if (strncmp("occ, ", raw, 5) == 0) {
    for (int i = 5; i < at; i++) {
      if (raw[i] != ' ')
        continue;
      i++;
      int occupancy = atoi(&raw[i]);
      for (auto *sensor : this->occupancy_sensors_) {
        sensor->update_occupancy(occupancy);
      }
      for (auto *sensor : this->motion_sensors_) {
        sensor->update_occupancy(occupancy);
      }
      break;
    }
  }

  return false;
}

}  // namespace cem5855h
}  // namespace esphome
