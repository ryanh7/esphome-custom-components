#include "fpm383c.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <cstring>

namespace esphome {
namespace fpm383c {

static const char *const TAG = "fpm383x";

#define UART_FRAME_FLAG 0xF1, 0x1F, 0xE2, 0x2E, 0xB6, 0x6B, 0xA8, 0x8A
#define DEFAULT_PASSWORD 0x00, 0x00, 0x00, 0x00
#define UART_WAIT_MS 100

static const uint8_t UART_FRAME_START[8] = {UART_FRAME_FLAG};

void FPM383cComponent::setup() { this->command_(0x03, 0x01); }

void FPM383cComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "fpm383x:");
  this->check_uart_settings(57600);
  ESP_LOGCONFIG(TAG, "  Model ID: %s", this->model_id_);
}

void FPM383cComponent::loop() {
  if (!this->available()) {
    return;
  }

  while (this->available()) {
    uint8_t byte;
    this->read_byte(&byte);
    if (this->parse_(byte)) {
      ESP_LOGVV(TAG, "Parsed: 0x%02X", byte);
    } else {
      this->rx_buffer_.clear();
    }
  }

  this->wait_at_ = micros() + (this->rx_buffer_.empty() ? 0 : UART_WAIT_MS);
}

void FPM383cComponent::update() {
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
      if (!this->have_wait_()) {
        this->command_(0x01, 0x22);
        this->status_ = STATUS_IDLE;
      }
      break;
    }
    default:
      break;
  }

  if (this->enable_auto_learning && this->renew_id_ != 0xFFFF && !this->have_wait_()) {
    // 自学习指纹
    std::vector<uint8_t> update_command = {DEFAULT_PASSWORD, 0x01, 0x16, (uint8_t) (this->renew_id_ >> 8),
                                           (uint8_t) (this->renew_id_ & 0xFF)};
    this->command_(update_command);
    this->renew_id_ = 0xFFFF;
  }

  // 查询手指在位
  if (!this->have_wait_()) {
    this->command_(0x01, 0x35);
  }
}

void FPM383cComponent::turn_on_light(Color color) {
  std::vector<uint8_t> command = {DEFAULT_PASSWORD, 0x02, 0x0F, 0x01, color, 0, 0, 0};
  this->command_(command);
}

void FPM383cComponent::turn_off_light() {
  std::vector<uint8_t> command = {DEFAULT_PASSWORD, 0x02, 0x0F, 0x00, 0, 0, 0, 0};
  this->command_(command);
}

void FPM383cComponent::breathing_light(Color color, uint8_t min_level, uint8_t max_level, uint8_t rate) {
  std::vector<uint8_t> command = {DEFAULT_PASSWORD, 0x02, 0x0F, 0x03, color, max_level, min_level, rate};
  this->command_(command);
}

void FPM383cComponent::flashing_light(Color color, uint8_t on_10ms, uint8_t off_10ms, uint8_t count) {
  std::vector<uint8_t> command = {DEFAULT_PASSWORD, 0x02, 0x0F, 0x04, color, on_10ms, off_10ms, count};
  this->command_(command);
}

void FPM383cComponent::register_fingerprint() {
  if (this->status_ == STATUS_REGISTING) {
    // 取消注册
    this->command_(0x01, 0x15);
  }
  // 自动注册
  std::vector<uint8_t> command = {DEFAULT_PASSWORD, 0x01, 0x18, 0x01, 6, 0xFF, 0xFF};
  this->command_(command);

  this->last_register_progress_time_ = micros();
  this->status_ = STATUS_REGISTING;
}

void FPM383cComponent::clear_fingerprint() {
  // 清除全部指纹
  std::vector<uint8_t> command = {DEFAULT_PASSWORD, 0x01, 0x31, 0x01, 0x00, 0x00};
  this->command_(command);
}

void FPM383cComponent::cancel() { this->command_(0x01, 0x15); }

void FPM383cComponent::reset() { this->command_(0x02, 0x02); }

int FPM383cComponent::parse_(uint8_t byte) {
  size_t at = this->rx_buffer_.size();
  this->rx_buffer_.push_back(byte);
  const uint8_t *raw = &this->rx_buffer_[0];

  static uint16_t frame_length = 0;

  // Start
  if (at == 0 && raw[at] == 0x55) {  // reset flag
    this->on_reset();
    return false;
  }

  if (at < sizeof(UART_FRAME_START))  // read frame header:F1,1F,E2,2E,B6,6B,A8,8A
    return byte == UART_FRAME_START[at];

  if (at < sizeof(UART_FRAME_START) + 1)  // read data length : 2 bytes
    return true;

  if (at == sizeof(UART_FRAME_START) + 1) {  //
    frame_length = encode_uint16(raw[at - 1], raw[at]) + sizeof(UART_FRAME_START) + 2 + 1;
    //                  ^data length                     ^header                  ^data length ^SOF checksum
    return frame_length > 17 && frame_length < 256;
  }

  if (at == sizeof(UART_FRAME_START) + 2) {  // read SOF checksum and check
    return raw[at] == checksum_(raw, at);
  }

  if (at < frame_length - 1)
    return true;

  // at last

  // check data checksum
  if (raw[at] != checksum_((raw + sizeof(UART_FRAME_START) + 3), (at - sizeof(UART_FRAME_START) - 3))) {
    ESP_LOGE(TAG, "Data checksum failed!");
    return false;
  }

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
    // ID
    case 0x0301: {
      for (uint8_t i = 0; i < 16; i++) {
        this->model_id_[i] = raw[21 + i];
      }
      this->model_id_[16] = 0x00;
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

void FPM383cComponent::on_reset() {
  for (auto *listener : this->reset_listeners_) {
    listener->on_reset();
  }
}

void FPM383cComponent::on_match_(bool sucessed, uint16_t id, uint16_t score) {
  this->renew_id_ = sucessed ? id : 0xFFFF;
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
  std::vector<uint8_t> command = {DEFAULT_PASSWORD, cmd1, cmd2};
  this->command_(command);
}

void FPM383cComponent::command_(std::vector<uint8_t> &data) {
  std::vector<uint8_t> command = {UART_FRAME_FLAG};
  uint16_t data_length = data.size() + 1;
  command.push_back((uint8_t) (data_length >> 8));
  command.push_back((uint8_t) (data_length & 0xFF));
  command.push_back(checksum_(&command[0], command.size()));
  command.insert(command.end(), data.begin(), data.end());
  command.push_back(checksum_(&data[0], data.size()));
  this->write_array(command);
  this->flush();
  this->wait_at_ = micros() + UART_WAIT_MS;
}

bool FPM383cComponent::have_wait_() { return micros() < this->wait_at_; }

uint8_t FPM383cComponent::checksum_(const uint8_t *data, const uint32_t length) {
  int8_t sum = 0;
  for (uint32_t i = 0; i < length; i++) {
    sum += data[i];
  }
  return (uint8_t) ((~sum) + 1);
}

}  // namespace fpm383c
}  // namespace esphome
