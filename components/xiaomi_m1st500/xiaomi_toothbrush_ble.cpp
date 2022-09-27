#include "xiaomi_toothbrush_ble.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP32

#include <vector>

namespace esphome {
namespace xiaomi_toothbrush_ble {

static const char *const TAG = "xiaomi_toothbrush_ble";

bool parse_xiaomi_value(uint16_t value_type, const uint8_t *data, uint8_t value_length, XiaomiParseResult &result) {
  // battery, 1 byte, 8-bit unsigned integer, 1 %
  if ((value_type == 0x0A10) && (value_length == 1)) {
    result.battery_level = data[0];
  } else if ((value_type == 0x1000) && (value_length == 2) && (data[0] == 0x01)) {
    result.score = data[1];
  } else {
    return false;
  }

  return true;
}

bool parse_xiaomi_message(const std::vector<uint8_t> &message, XiaomiParseResult &result) {
  result.has_encryption = (message[0] & 0x08) ? true : false;  // update encryption status
  if (result.has_encryption) {
    ESP_LOGVV(TAG, "parse_xiaomi_message(): payload is encrypted, stop reading message.");
    return false;
  }

  // Data point specs
  // Byte 0: type
  // Byte 1: fixed 0x10 or 0x00
  // Byte 2: length
  // Byte 3..3+len-1: data point value

  const uint8_t *payload = message.data() + result.raw_offset;
  uint8_t payload_length = message.size() - result.raw_offset;
  uint8_t payload_offset = 0;
  bool success = false;

  if (payload_length < 4) {
    ESP_LOGVV(TAG, "parse_xiaomi_message(): payload has wrong size (%d)!", payload_length);
    return false;
  }

  while (payload_length > 3) {
    const uint8_t value_length = payload[payload_offset + 2];
    if ((value_length < 1) || (value_length > 4) || (payload_length < (3 + value_length))) {
      ESP_LOGVV(TAG, "parse_xiaomi_message(): value has wrong size (%d)!", value_length);
      break;
    }

    const uint16_t value_type = payload[payload_offset + 0] << 8 | payload[payload_offset + 1];
    const uint8_t *data = &payload[payload_offset + 3];

    if (parse_xiaomi_value(value_type, data, value_length, result))
      success = true;

    payload_length -= 3 + value_length;
    payload_offset += 3 + value_length;
  }

  return success;
}

optional<XiaomiParseResult> parse_xiaomi_header(const esp32_ble_tracker::ServiceData &service_data) {
  XiaomiParseResult result;
  if (!service_data.uuid.contains(0x95, 0xFE)) {
    ESP_LOGVV(TAG, "parse_xiaomi_header(): no service data UUID magic bytes.");
    return {};
  }

  auto raw = service_data.data;
  result.has_data = (raw[0] & 0x40) ? true : false;
  result.has_capability = (raw[0] & 0x20) ? true : false;
  result.has_encryption = (raw[0] & 0x08) ? true : false;

  if (!result.has_data) {
    ESP_LOGVV(TAG, "parse_xiaomi_header(): service data has no DATA flag.");
    return {};
  }

  static uint8_t last_frame_count = 0;
  if (last_frame_count == raw[4]) {
    ESP_LOGVV(TAG, "parse_xiaomi_header(): duplicate data packet received (%d).", static_cast<int>(last_frame_count));
    result.is_duplicate = true;
    return {};
  }
  last_frame_count = raw[4];
  result.is_duplicate = false;
  result.raw_offset = result.has_capability ? 12 : 11;

  if ((raw[2] == 0x89) && (raw[3] == 0x04)) {  // MiFlora
    result.type = XiaomiParseResult::TYPE_M1ST500;
    result.name = "M1S-T500";
  } else {
    ESP_LOGVV(TAG, "parse_xiaomi_header(): unknown device, no magic bytes.");
    return {};
  }

  return result;
}

bool report_xiaomi_results(const optional<XiaomiParseResult> &result, const std::string &address) {
  if (!result.has_value()) {
    ESP_LOGVV(TAG, "report_xiaomi_results(): no results available.");
    return false;
  }

  ESP_LOGD(TAG, "Got Xiaomi %s (%s):", result->name.c_str(), address.c_str());

  if (result->score.has_value()) {
    ESP_LOGD(TAG, "  Score: %.0f", *result->score);
  }
  if (result->battery_level.has_value()) {
    ESP_LOGD(TAG, "  Battery Level: %.0f%%", *result->battery_level);
  }

  return true;
}

}  // namespace xiaomi_toothbrush_ble
}  // namespace esphome

#endif
