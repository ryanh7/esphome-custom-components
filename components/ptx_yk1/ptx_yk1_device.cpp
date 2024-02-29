#include "ptx_yk1_device.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace ptx_yk1 {

static const char *const TAG = "ptx_yk1";

void PTXYK1Device::dump_config() { 
    LOG_BINARY_SENSOR("", "PTX YK1", this);
    ESP_LOGCONFIG(TAG, "  BLE Timeout: %dms", this->timeout_ms_);
}

void PTXYK1Device::loop() {
  uint32_t now = millis();
  if (this->need_timeout_ && ((now < this->time_) || (now - this->time_) > this->timeout_ms_)) {
    this->publish_state(false);
    this->need_timeout_ = false;
  }
}

bool PTXYK1Device::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (device.address_uint64() != this->address_) {
    return false;
  }
  for (auto adv : device.get_manufacturer_datas()) {
    if (adv.uuid != esp32_ble_tracker::ESPBTUUID::from_uint16(0x5348) || adv.data.size() != 14) {
      continue;
    }
    this->time_ = millis();
    uint32_t id = encode_uint24(adv.data[11], adv.data[12], adv.data[13]);
    if (id == this->last_id_) {
      return true;
    }
    if (this->state) {
      this->publish_state(false);
      this->need_timeout_ = true;
    }
    this->last_id_ = id;
    this->publish_state(true);
    this->need_timeout_ = true;
    return true;
  }
  return true;
}

}  // namespace ptx_yk1
}  // namespace esphome

#endif
