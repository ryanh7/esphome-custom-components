#include "bt_presence_device.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace bt_presence {

static const char *const TAG = "bt_presence";

void BTPresenceDevice::dump_config() { LOG_BINARY_SENSOR("", "BT Presence", this); }

}  // namespace bt_presence
}  // namespace esphome

#endif
