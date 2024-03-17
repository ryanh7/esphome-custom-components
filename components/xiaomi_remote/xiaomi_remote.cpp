#include "xiaomi_remote.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace xiaomi_remote {

static const char *const TAG = "xiaomi_remote";

void XiaomiRemote::dump_config() {
  ESP_LOGCONFIG(TAG, "Xiaomi Remote");
  LOG_SENSOR("  ", "Battery Level", this->battery_level_);
}

bool XiaomiRemote::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (device.address_uint64() != this->address_) {
    ESP_LOGVV(TAG, "parse_device(): unknown MAC address.");
    return false;
  }
  ESP_LOGVV(TAG, "parse_device(): MAC address %s found.", device.address_str().c_str());

  for (auto &service_data : device.get_service_datas()) {
    auto res = parse_xiaomi_header(service_data);
    if (!res.has_value()) {
      continue;
    }
    if (res->is_duplicate) {
      continue;
    }
    if (res->has_encryption) {
      if (!this->bindkey_.has_value()) {
        ESP_LOGE(TAG, "need bindkey!");
        continue;
      }
      if (!(decrypt_xiaomi_payload(const_cast<std::vector<uint8_t> &>(service_data.data), (*this->bindkey_).data(), this->address_))) {
        continue;
      }
    }
    if (!(parse_xiaomi_message(service_data.data, *res))) {
      continue;
    }
    if (!(report_xiaomi_results(res, device.address_str()))) {
      continue;
    }
    if (res->button.has_value())
      for (auto *listener : listeners_) {
        listener->on_change(*res->button, *res->action);
      }
    if (res->battery_level.has_value() && this->battery_level_ != nullptr)
      this->battery_level_->publish_state(*res->battery_level);
  }

  return true;
}

void XiaomiRemote::set_bindkey(const std::string &bindkey) {
  if (bindkey.size() != 32) {
    return;
  }
  std::array<uint8_t, 16> keys;
  char temp[3] = {0};
  for (int i = 0; i < 16; i++) {
    strncpy(temp, &(bindkey.c_str()[i * 2]), 2);
    keys[i] = std::strtoul(temp, nullptr, 16);
  }
  this->bindkey_ = keys;
}

void RemoteSensor::on_change(uint8_t action) {
  switch (action) {
    case BUTTON_TYPE_CLICK:
      this->publish_state(1);
      break;
    case BUTTON_TYPE_DOUBLE_CLICK:
      this->publish_state(2);
      break;
    case BUTTON_TYPE_TRIPLE_CLICK:
      this->publish_state(3);
      break;
    case BUTTON_TYPE_LONG_PRESS:
      this->publish_state(99);
      break;
    default:
      break;
  }
  this->publish_state(0);
}

void RemoteTextSensor::on_change(uint8_t action) {
  switch (action) {
    case BUTTON_TYPE_CLICK:
      this->publish_state("Click");
      break;
    case BUTTON_TYPE_DOUBLE_CLICK:
      this->publish_state("Double-click");
      break;
    case BUTTON_TYPE_TRIPLE_CLICK:
      this->publish_state("Triple-click");
      break;
    case BUTTON_TYPE_LONG_PRESS:
      this->publish_state("Long press");
      break;
    default:
      this->publish_state("Unkown");
      break;
  }
  this->publish_state("Idle");
}

}  // namespace xiaomi_remote
}  // namespace esphome

#endif
