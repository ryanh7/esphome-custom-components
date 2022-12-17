#include "dlt645.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dlt645 {

static const char *const TAG = "DL/T 645";

#define BIT_TIME 833

void DLT645Component::dump_config() {
  ESP_LOGCONFIG(TAG, "DL/T 645:");
  if (!this->address_.empty()) {
    ESP_LOGCONFIG(TAG, "  Address: %02X%02X%02X%02X%02X%02X", this->address_[5], this->address_[4], this->address_[3],
                  this->address_[2], this->address_[1], this->address_[0]);
  } else {
    ESP_LOGCONFIG(TAG, "  Address: Unkown");
  }
  if (this->power_sensor_ != nullptr) {
    LOG_SENSOR("  ", "Power", this->power_sensor_);
  }
  if (this->energy_sensor_ != nullptr) {
    LOG_SENSOR("  ", "Energy", this->energy_sensor_);
  }
}

void DLT645Component::setup() {}

void DLT645Component::loop() {
  if (!rx_buffer_.empty()) {
    for (int i = 0; i < this->rx_buffer_.size(); i++) {
      if (this->rx_buffer_[i] == 0x68) {
        if (i + 11 >= this->rx_buffer_.size()) {
          continue;
        }
        if (this->rx_buffer_[i + 7] != 0x68) {
          continue;
        }
        int end_at = i + 11 + this->rx_buffer_[i + 9];
        if (end_at >= this->rx_buffer_.size() || this->rx_buffer_[end_at] != 0x16) {
          continue;
        }
        if (this->rx_buffer_[end_at - 1] != this->checksum_(this->rx_buffer_, i, end_at - i - 1)) {
          continue;
        }
        for (int j = i + 10; j < end_at - 1; j++) {
          this->rx_buffer_[j] -= 0x33;
        }
        this->handle_response(this->rx_buffer_, i + 8);
        i = end_at;
      }
    }
    this->rx_buffer_.clear();
  }

  if (this->address_.empty()) {
    static uint32_t last_request = 0;
    if (last_request > 0 && micros() < last_request + 1000000) {
      return;
    }
    last_request = micros();
    std::vector<uint8_t> address = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    this->send(address, 0x13, nullptr);
    return;
  }

  if (this->power_sensor_ != nullptr && this->power_sensor_->is_timeout()) {
    std::vector<uint8_t> data_type = {0x00, 0x00, 0x03, 0x02};
    this->send(this->address_, 0x11, &data_type, 200);
    this->power_sensor_->last_update = micros();
  }

  if (this->energy_sensor_ != nullptr && this->energy_sensor_->is_timeout()) {
    std::vector<uint8_t> data_type = {0x00, 0x00, 0x00, 0x00};
    this->send(this->address_, 0x11, &data_type, 200);
    this->energy_sensor_->last_update = micros();
  }
}

static uint8_t decode_byte(uint8_t byte) { return (byte >> 4) * 10 + (byte & 0x0F); }

void DLT645Component::handle_response(std::vector<uint8_t> &data, int index) {
  uint8_t opcode = data[index];
  if (opcode == 0x93) {
    this->address_.clear();
    for (int i = index + 2; i < index + 8; i++) {
      this->address_.push_back(data[i]);
    }
    return;
  }
  if (opcode == 0x91) {
    uint32_t data_type = encode_uint32(data[index + 5], data[index + 4], data[index + 3], data[index + 2]);
    switch (data_type) {
      case 0x02030000: {
        if (data[index + 1] == 1) {
          ESP_LOGE(TAG, "Power error: %02X", data[index + 2]);
          break;
        }
        if (data[index + 1] != 7) {
          ESP_LOGE(TAG, "Power length invaild: %d", data[index + 1]);
          break;
        }
        if (this->power_sensor_ != nullptr) {
          float value = decode_byte(data[index + 8] & 0x7F) + decode_byte(data[index + 7]) / 100.0f +
                        decode_byte(data[index + 6]) / 10000.0f;
          this->power_sensor_->publish_state(value);
          this->power_sensor_->last_update = micros();
        }
        break;
      }
      case 0x00000000: {
        if (data[index + 1] == 1) {
          ESP_LOGE(TAG, "Energy error: %02X", data[index + 2]);
          break;
        }
        if (data[index + 1] != 8) {
          ESP_LOGE(TAG, "Engery length invaild: %d", data[index + 1]);
          break;
        }
        if (this->energy_sensor_ != nullptr) {
          float value = decode_byte(data[index + 9] & 0x7F) * 10000.0f + decode_byte(data[index + 8]) * 100.0f +
                        decode_byte(data[index + 7]) + decode_byte(data[index + 6]) / 100.0f;
          this->energy_sensor_->publish_state(value);
          this->energy_sensor_->last_update = micros();
        }
        break;
      }
      default:
        ESP_LOGW(TAG, "Unkown type: %08X", data_type);
        break;
    }
  }
}

void DLT645Component::set_address(const std::string &address) {
  this->address_.clear();
  if (address.size() != 12) {
    return;
  }
  char temp[3] = {0};
  for (int i = 5; i >= 0; i--) {
    strncpy(temp, &(address.c_str()[i * 2]), 2);
    this->address_.push_back(std::strtoul(temp, nullptr, 16));
  }
}

bool DLT645Component::on_receive(remote_base::RemoteReceiveData data) {
  std::vector<int32_t> *raw = data.get_raw_data();
  uint8_t bits = 0;
  int bits_at = 0;
  bool parity = false;
  for (int32_t time : *raw) {
    uint8_t bit = time > 0 ? 0 : 1;
    time = (time > 0 ? time : -time) + 400;
    for (time -= BIT_TIME; time > 0; time -= BIT_TIME, bits_at++) {
      if (bits_at == 0) {
        if (bit) {
          break;
        }
      } else if (bits_at < 9) {
        bits |= bit << (bits_at - 1);
        parity ^= bit;
      } else if (bits_at == 9) {
        if (bit == parity) {
          this->rx_buffer_.push_back(bits);
        }
      } else {
        bits_at = 0;
        bits = 0;
        parity = false;
        break;
      }
    }
  }
  return false;
}

void DLT645Component::send(std::vector<uint8_t> &address, uint8_t opcode, std::vector<uint8_t> *data,
                           uint16_t wait_ms) {
  std::vector<uint8_t> request = {0xFE, 0xFE, 0xFE, 0xFE, 0x68};
  request.insert(request.end(), address.begin(), address.end());
  request.push_back(0x68);
  request.push_back(opcode);
  request.push_back(data == nullptr ? 0 : data->size());
  if (data != nullptr) {
    for (int i = 0; i < data->size(); i++) {
      request.push_back((*data)[i] + 0x33);
    }
  }
  request.push_back(this->checksum_(request, 4, request.size() - 4));
  request.push_back(0x16);

  uint32_t now = micros();
  if (now < this->next_time_) {
    delay((this->next_time_ - now) / 1000);
  }

  auto transmit = this->transmitter_->transmit();
  auto *transmit_data = transmit.get_data();

  transmit_data->set_carrier_frequency(38000);
  bool last_bit = false;
  int32_t time = 0;
  for (auto byte : request) {
    bool parity_bit = true;
    for (int i = 0; i < 11; i++) {
      bool bit = false;
      switch (i) {
        case 0:
          bit = false;
          break;
        case 9:
          bit = parity_bit;
          break;
        case 10:
          bit = true;
          break;
        default:
          bit = (byte & (1 << (i - 1)));
          break;
      }
      if (bit == last_bit) {
        time += BIT_TIME;
      } else {
        if (last_bit) {
          transmit_data->space(time);
        } else {
          transmit_data->mark(time);
        }
        time = BIT_TIME;
        last_bit = bit;
      }
    }
  }
  transmit_data->space(time);

  transmit.perform();
  this->next_time_ = micros() + wait_ms * 1000;
}

uint8_t DLT645Component::checksum_(std::vector<uint8_t> &data, int start, int length) {
  uint8_t sum = 0;
  for (int i = start; i < start + length && i < data.size(); i++) {
    sum += data[i];
  }
  return sum;
}

}  // namespace dlt645
}  // namespace esphome
