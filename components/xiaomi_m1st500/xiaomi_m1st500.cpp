#include "xiaomi_m1st500.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace xiaomi_m1st500 {

static const char *const TAG = "xiaomi_m1st500";

void XiaomiM1ST500::dump_config() {
  ESP_LOGCONFIG(TAG, "Xiaomi M1ST500");
  LOG_SENSOR("  ", "Score", this->score_);
  LOG_SENSOR("  ", "Battery Level", this->battery_level_);
}

bool XiaomiM1ST500::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (device.address_uint64() != this->address_) {
    ESP_LOGVV(TAG, "parse_device(): unknown MAC address.");
    return false;
  }
  ESP_LOGVV(TAG, "parse_device(): MAC address %s found.", device.address_str().c_str());

  bool success = false;
  for (auto &service_data : device.get_service_datas()) {
    auto res = xiaomi_toothbrush_ble::parse_xiaomi_header(service_data);
    if (!res.has_value()) {
      continue;
    }
    if (res->is_duplicate) {
      continue;
    }
    if (res->has_encryption) {
      ESP_LOGVV(TAG, "parse_device(): payload decryption is currently not supported on this device.");
      continue;
    }
    if (!(xiaomi_toothbrush_ble::parse_xiaomi_message(service_data.data, *res))) {
      continue;
    }
    if (!(xiaomi_toothbrush_ble::report_xiaomi_results(res, device.address_str()))) {
      continue;
    }
    if (res->score.has_value() && this->score_ != nullptr)
      this->score_->publish_state(*res->score);
    if (res->battery_level.has_value() && this->battery_level_ != nullptr)
      this->battery_level_->publish_state(*res->battery_level);
    success = true;
  }

  if (!success) {
    return false;
  }

  return true;
}

}  // namespace xiaomi_m1st500
}  // namespace esphome

#endif
