#include "ble_tracker.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace ble_tracker {

static const char *const TAG = "ble_tracker";

void BLETracker::dump_config() { ESP_LOGCONFIG(TAG, "BLE_Tracker:"); }

void BLETracker::setup() { this->station_ = get_mac_address_pretty(); }

void BLETracker::update() {
  if (this->rssi_datas_.empty())
    return;

  std::string rssi_info;
  for (auto data : this->rssi_datas_) {
    rssi_info += data.address;
    switch (data.address_type) {
      case BLE_ADDR_TYPE_PUBLIC:
        rssi_info += "/P/";
        break;
      case BLE_ADDR_TYPE_RANDOM:
        rssi_info += "/R/";
        break;
      case BLE_ADDR_TYPE_RPA_PUBLIC:
        rssi_info += "/RP/";
        break;
      case BLE_ADDR_TYPE_RPA_RANDOM:
        rssi_info += "/RR/";
        break;
    }
    rssi_info += to_string(data.rssi) + ",";
  }
  rssi_info.pop_back();
  this->rssi_datas_.clear();

  this->fire_homeassistant_event("esphome.ble_tracker", {{"station", this->station_}, {"devices", rssi_info}});
}

}  // namespace ble_tracker
}  // namespace esphome

#endif
