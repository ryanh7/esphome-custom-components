#pragma once

#include "esphome/core/component.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#ifdef USE_ESP32

namespace esphome {
namespace xiaomi_toothbrush_ble {

struct XiaomiParseResult {
  enum {
    TYPE_M1ST500
  } type;
  std::string name;
  optional<float> score;
  optional<float> battery_level;
  bool has_data;        // 0x40
  bool has_capability;  // 0x20
  bool has_encryption;  // 0x08
  bool is_duplicate;
  int raw_offset;
};


bool parse_xiaomi_value(uint8_t value_type, const uint8_t *data, uint8_t value_length, XiaomiParseResult &result);
bool parse_xiaomi_message(const std::vector<uint8_t> &message, XiaomiParseResult &result);
optional<XiaomiParseResult> parse_xiaomi_header(const esp32_ble_tracker::ServiceData &service_data);
bool report_xiaomi_results(const optional<XiaomiParseResult> &result, const std::string &address);


}  // namespace xiaomi_ble
}  // namespace esphome

#endif
