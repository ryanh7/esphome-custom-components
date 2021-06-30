#include "ssw_tds.h"
#include "esphome/core/log.h"
#include <cstring>

namespace esphome {
namespace ssw_tds {

static const char *const TAG = "ssw_tds";

void SSWTDSComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SSW_TDS:");
  this->check_uart_settings(9600);
  LOG_SENSOR("  ", "Source TDS", this->source_tds_sensor_);
  LOG_SENSOR("  ", "Clean TDS", this->clean_tds_sensor_);
  LOG_SENSOR("  ", "Temperature TDS", this->temperature_sensor_);
}

void SSWTDSComponent::update() {
  this->query_();

  delay(200);

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

void SSWTDSComponent::query_() {
  // Empty RX Buffer
  while (this->available())
    this->read();
  this->write_array(URAT_QUERY_COMMAND, sizeof(URAT_QUERY_COMMAND));
  this->flush();
}

int SSWTDSComponent::parse_(uint8_t byte) {
  size_t at = this->rx_buffer_.size();
  this->rx_buffer_.push_back(byte);
  const uint8_t *raw = &this->rx_buffer_[0];

  ESP_LOGVV(TAG, "Processing byte: 0x%02X", byte);

  // Byte 0: Start
  if (at == 0)
    return byte == URAT_FEEDBACK_START;

  // Byte 1: Action
  if (at == 1)
    return byte == URAT_QUERY_COMMAND[1];

  if (at < 12)
    return true;

  if (!this->checksum_()) {
    ESP_LOGE(TAG, "checksum failed!");
    return false;
  }

  int source_tds = raw[2] << 24 | raw[3] << 16 | raw[4] << 8 | raw[5];
  int clean_tds = raw[6] << 24 | raw[7] << 16 | raw[8] << 8 | raw[9];
  int temperature = raw[10];

  ESP_LOGD(TAG, "got source tds=%d, clean tds=%d, temperature=%d", source_tds, clean_tds, temperature);

  if (this->source_tds_sensor_ != nullptr) {
    this->source_tds_sensor_->publish_state(source_tds);
  }

  if (this->clean_tds_sensor_ != nullptr) {
    this->clean_tds_sensor_->publish_state(clean_tds);
  }

  if (this->temperature_sensor_ != nullptr) {
    this->temperature_sensor_->publish_state(temperature);
  }

  return false;
}

int SSWTDSComponent::checksum_() {
  if (this->rx_buffer_.size() < 13)
    return false;
  const uint8_t *raw = &this->rx_buffer_[0];
  uint16_t calc = 0;
  for (int i = 0; i < 11; i++) {
    calc += raw[i];
  }
  return calc == (raw[11] << 8 | raw[12]);
}

}  // namespace ssw_tds
}  // namespace esphome
