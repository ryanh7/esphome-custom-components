#include "bt_rssi_sensor.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace bt_rssi {

static const char *const TAG = "bt_rssi";

void BTRSSISensor::dump_config() { LOG_SENSOR("", "BT RSSI Sensor", this); }

}  // namespace ble_rssi
}  // namespace esphome

#endif
