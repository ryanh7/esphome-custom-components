#include "dlt645.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dlt645 {

static const char *const TAG = "DL/T 645";

#define BIT_TIME 833
#define SEND_INTERVAL 500000

void DLT645Component::dump_config() {
  ESP_LOGCONFIG(TAG, "DL/T 645:");
  if (!this->address_.empty()) {
    ESP_LOGCONFIG(TAG, "  Address: %02X%02X%02X%02X%02X%02X", this->address_[5], this->address_[4], this->address_[3],
                  this->address_[2], this->address_[1], this->address_[0]);
  } else {
    ESP_LOGCONFIG(TAG, "  Address: Unkown");
  }
  for (auto sensor : this->sensors_) {
    ESP_LOGCONFIG(TAG, "  %s", sensor->get_type().c_str());
    LOG_SENSOR("  ", "  Name:", sensor);
  }
}

void DLT645Component::setup() {}

void DLT645Component::loop() {
  if (micros() < this->next_time_ && this->next_time_ - micros() < SEND_INTERVAL) {
    return;
  }

  if (this->address_.empty()) {
    std::vector<uint8_t> address = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    this->send(address, 0x13);
    return;
  }

  static int next = -1;
  size_t size = this->sensors_.size();
  if (size == 0) {
    return;
  }
  for (int i = 0; i < size; i++) {
    next = (next + 1) % size;
    if (this->sensors_[next]->is_timeout()) {
      this->send(this->address_, 0x11, this->sensors_[next]->get_id());
      break;
    }
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
    for (auto sensor : this->sensors_) {
      if (data_type == sensor->get_id()) {
        uint32_t value = 0;
        uint8_t fraction = ((uint8_t) (sensor->get_format() * 10.0f)) % 10;
        uint8_t integer = ((uint8_t) sensor->get_format()) % 10;
        data[index + 5 + fraction + integer] &= 0x7F;
        for (uint8_t i = fraction + integer; i > 0; i--) {
          value = value * 100 + decode_byte(data[index + 5 + i]);
        }
        sensor->update(value * pow(0.01, fraction));
        break;
      }
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
        this->next_time_ = micros();
      }
    }
    this->rx_buffer_.clear();
  }
  return false;
}

void DLT645Component::send(std::vector<uint8_t> &address, uint8_t opcode, uint32_t data_id) {
  std::vector<uint8_t> request = {0xFE, 0xFE, 0xFE, 0xFE, 0x68};
  request.insert(request.end(), address.begin(), address.end());
  request.push_back(0x68);
  request.push_back(opcode);
  if (data_id != 0xFFFFFFFF) {
    request.push_back(4);
    for (int i = 0; i < 4; i++) {
      request.push_back(((data_id >> (i * 8)) & 0xFF) + 0x33);
    }
  } else {
    request.push_back(0);
  }
  request.push_back(this->checksum_(request, 4, request.size() - 4));
  request.push_back(0x16);

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
  this->next_time_ = micros() + SEND_INTERVAL;
}

uint8_t DLT645Component::checksum_(std::vector<uint8_t> &data, int start, int length) {
  uint8_t sum = 0;
  for (int i = start; i < start + length && i < data.size(); i++) {
    sum += data[i];
  }
  return sum;
}

bool DLT645Sensor::is_timeout() { return (micros() < last_update) || ((micros() - last_update) > update_interval); }

void DLT645Sensor::update(float state) {
  this->publish_state(state);
  this->last_update = micros();
}

}  // namespace dlt645
}  // namespace esphome
