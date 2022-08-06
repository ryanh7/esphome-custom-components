#include "fpm383c.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <cstring>

namespace esphome {
namespace fpm383c {

static const char *const TAG = "fpm383c";

#define UART_FRAME_FLAG 0xF1, 0x1F, 0xE2, 0x2E, 0xB6, 0x6B, 0xA8, 0x8A
#define DEFAULT_PASSWORD 0x00, 0x00, 0x00, 0x00

static const uint8_t UART_FRAME_START[8] = {UART_FRAME_FLAG};

void FPM383cComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "fpm383c:");
  this->check_uart_settings(57600);
}

void FPM383cComponent::loop() {
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

void FPM383cComponent::set_light_level(Color color, float level) {
  // Empty RX Buffer
  /*   while (this->available())
      this->read(); */
  uint8_t level_pecent = level * 100;
  uint8_t command[] = {UART_FRAME_FLAG, 0x00,         0x0C, 0x81, DEFAULT_PASSWORD, 0x02, 0x0F, 0x03, color,
                       level_pecent,    level_pecent, 0,    0};
  command[sizeof(command) - 1] = checksum_(command + 11, sizeof(command) - 12);
  this->write_array(command, sizeof(command));
  this->flush();
}

int FPM383cComponent::parse_(uint8_t byte) {
  size_t at = this->rx_buffer_.size();
  this->rx_buffer_.push_back(byte);
  const uint8_t *raw = &this->rx_buffer_[0];

  ESP_LOGVV(TAG, "Processing byte: 0x%02X, at=%d", byte, at);

  static uint16_t frame_length = 0;

  // Start
  if (at < sizeof(UART_FRAME_START))
    return byte == UART_FRAME_START[at];

  if (at < 9)
    return true;

  if (at == 9) {
    frame_length = encode_uint16(raw[8], raw[9]) + 11;
    ESP_LOGD(TAG, "frame length=%d", frame_length);
    return frame_length > 17 && frame_length < 30;
  }

  if (at < frame_length - 1)
    return true;

  if (!this->check_()) {
    ESP_LOGE(TAG, "checksum failed!");
    return false;
  }

  ESP_LOGD(TAG, "response: ACK=%02X %02X, error=%02X%02X%02X%02X", raw[15], raw[16], raw[17], raw[18], raw[19],
           raw[20]);

  // TODO: 检查返回值
  return false;
}

uint8_t FPM383cComponent::checksum_(uint8_t *data, uint32_t length) {
  uint32_t i = 0;
  int8_t sum = 0;
  for (i = 0; i < length; i++) {
    sum += data[i];
  }
  return (uint8_t) ((~sum) + 1);
}

bool FPM383cComponent::check_() {
  size_t buffer_length = this->rx_buffer_.size();
  if (buffer_length < 12)
    return false;

  uint8_t *raw = &this->rx_buffer_[0];

  if (raw[10] != checksum_(raw, 10))
    return false;

  uint16_t user_raw_length = encode_uint16(raw[8], raw[9]);
  if (user_raw_length + 11 > buffer_length)
    return false;

  uint8_t *user_raw = raw + 11;
  if (user_raw[user_raw_length - 1] != checksum_(user_raw, user_raw_length - 1))
    return false;

  // TODO: 检查密码
  return true;
}

}  // namespace fpm383c
}  // namespace esphome
