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

void FPM383cComponent::update() {
  // 查询手指在位
  this->command_(0x01, 0x35);

  switch (this->status_) {
    case STATUS_REGISTING: {
      uint32_t current_time = micros();
      if (current_time - this->last_register_progress_time_ > 15000) {
        this->status_ = STATUS_IDLE;
      }
      break;
    }
    case STATUS_MATCHING: {
      // 查询匹配结果
      this->command_(0x01, 0x22);
      this->status_ = STATUS_IDLE;
      break;
    }
    default:
      break;
  }
}

void FPM383cComponent::set_light_level(Color color, float level) {
  uint8_t level_pecent = level * 100;
  uint8_t command[] = {UART_FRAME_FLAG, 0x00,         0x0C, 0x81, DEFAULT_PASSWORD, 0x02, 0x0F, 0x03, color,
                       level_pecent,    level_pecent, 0,    0};
  command[sizeof(command) - 1] = checksum_(command + 11, sizeof(command) - 12);
  this->write_array(command, sizeof(command));
  this->flush();
}

void FPM383cComponent::register_fingerprint() {
  if (this->status_ == STATUS_REGISTING) {
    // 取消注册
    this->command_(0x01, 0x15);
  }
  // 自动注册
  uint8_t command[] = {UART_FRAME_FLAG, 0x00, 0x0B, 0x82, DEFAULT_PASSWORD, 0x01, 0x18, 0x01, 6, 0xFF, 0xFF, 0};
  command[sizeof(command) - 1] = checksum_(command + 11, sizeof(command) - 12);
  this->write_array(command, sizeof(command));
  this->flush();

  this->last_register_progress_time_ = micros();
  this->status_ = STATUS_REGISTING;
}

void FPM383cComponent::clear_fingerprint() {
  // 清除全部指纹
  uint8_t command[] = {UART_FRAME_FLAG, 0x00, 0x0A, 0x83, DEFAULT_PASSWORD, 0x01, 0x31, 0x01, 0x00, 0x00, 0};
  command[sizeof(command) - 1] = checksum_(command + 11, sizeof(command) - 12);
  this->write_array(command, sizeof(command));
  this->flush();
}

void FPM383cComponent::cancel() {
  this->command_(0x01, 0x15);
}

void FPM383cComponent::reset() {
  this->command_(0x02, 0x02);
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
    return frame_length > 17 && frame_length < 30;
  }

  if (at < frame_length - 1)
    return true;

  if (!this->check_()) {
    ESP_LOGE(TAG, "checksum failed! ACK:%02X %02X", raw[15], raw[16]);
    return false;
  }

  ESP_LOGV(TAG, "response: ACK=%02X %02X, error=%02X%02X%02X%02X", raw[15], raw[16], raw[17], raw[18], raw[19],
           raw[20]);

  uint16_t ack = encode_uint16(raw[15], raw[16]);
  switch (ack) {
    // 指纹在位
    case 0x0135: {
      uint8_t status = raw[21];
      this->on_touch_(status == 1);
      break;
    }
    // 自动注册结果
    case 0x0118: {
      this->last_register_progress_time_ = micros();
      uint16_t id = encode_uint16(raw[22], raw[23]);
      this->on_register_progress_(id, raw[21], raw[24]);
      break;
    }
    // 匹配结果
    case 0x0122: {
      uint32_t error = encode_uint32(raw[17], raw[18], raw[19], raw[20]);
      if (error == 0) {
        uint16_t sucessed = encode_uint16(raw[21], raw[22]);
        uint16_t score = encode_uint16(raw[23], raw[24]);
        uint16_t id = encode_uint16(raw[25], raw[26]);
        ESP_LOGV(TAG, "sucessed: %04X, id:%04X, score:%04X", sucessed, id, score);
        on_match_((id != 0xFFFF), id, score);
      } else if (error == 0x04) {
        this->status_ = STATUS_MATCHING;
      } else {
        on_match_(false, 0, 0);
      }
      break;
    }
    // 重置
    case 0x0202: {
      this->status_ = STATUS_IDLE;
      break;
    }
    default:
      break;
  }

  // TODO: 检查密码
  return false;
}

void FPM383cComponent::on_touch_(bool touched) {
  if (touched == this->flag_touched_)
    return;
  this->flag_touched_ = touched;

  if (touched) {
    if (this->status_ == STATUS_IDLE) {
      this->command_(0x01, 0x21);
      this->status_ = STATUS_MATCHING;
    }
  } else if (this->status_ == STATUS_MATCHING) {
    this->status_ = STATUS_IDLE;
  }

  for (auto *listener : this->touch_listeners_) {
    listener->on_touch(touched);
  }
}

void FPM383cComponent::on_match_(bool sucessed, uint16_t id, uint16_t score) {
  for (auto *listener : this->fingerprint_match_listeners_) {
    listener->on_match(sucessed, id, score);
  }
}

void FPM383cComponent::on_register_progress_(uint16_t id, uint8_t step, uint8_t progress_in_percent) {
  if (progress_in_percent >= 100 && this->status_ == STATUS_REGISTING) {
    this->status_ = STATUS_IDLE;
  }
  if (step == 0xFF)
    return;
  for (auto *listener : this->fingerprint_register_listeners_) {
    listener->on_progress(id, step, progress_in_percent);
  }
}

void FPM383cComponent::command_(uint8_t cmd1, uint8_t cmd2) {
  uint8_t command[] = {UART_FRAME_FLAG, 0x00, 0x07, 0x86, DEFAULT_PASSWORD, cmd1, cmd2, 0};
  command[sizeof(command) - 1] = checksum_(command + 11, sizeof(command) - 12);
  this->write_array(command, sizeof(command));
  this->flush();
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

  if (raw[10] != checksum_(raw, 10)) {
    return false;
  }

  uint16_t user_raw_length = encode_uint16(raw[8], raw[9]);
  if (user_raw_length + 11 > buffer_length) {
    return false;
  }

  uint8_t *user_raw = raw + 11;
  if (user_raw[user_raw_length - 1] != checksum_(user_raw, user_raw_length - 1)) {
    if (!(raw[15] == 0x01 && raw[16] == 0x22)) {  // fpm383c的bug
      return false;
    }
  }

  // TODO: 检查密码
  return true;
}

}  // namespace fpm383c
}  // namespace esphome
