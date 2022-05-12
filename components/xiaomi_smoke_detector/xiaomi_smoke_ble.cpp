#include "xiaomi_smoke_ble.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

#include <vector>
#include "mbedtls/ccm.h"

namespace esphome {
namespace xiaomi_smoke_ble {

static const char *const TAG = "xiaomi_smoke_ble";

bool parse_xiaomi_value(uint16_t value_type, const uint8_t *data, uint8_t value_length, XiaomiParseResult &result) {
  if ((value_type == 0x100A) && (value_length == 1)) {
    result.battery_level = data[0];
  } else if ((value_type == 0x000D || value_type == 0x1015) && (value_length == 1)) {
    result.event = data[0];
  } else {
    return false;
  }

  return true;
}

bool parse_xiaomi_message(const std::vector<uint8_t> &message, XiaomiParseResult &result) {
  result.has_encryption = message[0] & 0x08;  // update encryption status
  if (result.has_encryption) {
    ESP_LOGVV(TAG, "parse_xiaomi_message(): payload is encrypted, stop reading message.");
    return false;
  }

  // Data point specs
  // Byte 0: type
  // Byte 1: fixed 0x10
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
    if (payload[payload_offset + 1] != 0x10 && payload[payload_offset + 1] != 0x00) {
      ESP_LOGVV(TAG, "parse_xiaomi_message(): fixed byte not found, stop parsing residual data.");
      break;
    }

    const uint8_t value_length = payload[payload_offset + 2];
    if ((value_length < 1) || (value_length > 4) || (payload_length < (3 + value_length))) {
      ESP_LOGVV(TAG, "parse_xiaomi_message(): value has wrong size (%d)!", value_length);
      break;
    }

    const uint16_t value_type = encode_uint16(payload[payload_offset + 1], payload[payload_offset + 0]);
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
  result.has_data = raw[0] & 0x40;
  result.has_capability = raw[0] & 0x20;
  result.has_encryption = raw[0] & 0x08;

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

  const uint16_t device_uuid = encode_uint16(raw[3], raw[2]);

  if (device_uuid == 0x0997) {  // Mi Smoke Detector
    result.type = XiaomiParseResult::TYPE_SMOKE_MCN02;
    result.name = "Smoke Detector";
  } else {
    ESP_LOGVV(TAG, "parse_xiaomi_header(): unknown device, no magic bytes.");
    return {};
  }

  return result;
}

bool decrypt_xiaomi_payload(std::vector<uint8_t> &raw, const uint8_t *bindkey, const uint64_t &address) {
  if (!((raw.size() == 19) || ((raw.size() >= 22) && (raw.size() <= 24)))) {
    ESP_LOGVV(TAG, "decrypt_xiaomi_payload(): data packet has wrong size (%d)!", raw.size());
    ESP_LOGVV(TAG, "  Packet : %s", format_hex_pretty(raw.data(), raw.size()).c_str());
    return false;
  }

  uint8_t mac_reverse[6] = {0};
  mac_reverse[5] = (uint8_t) (address >> 40);
  mac_reverse[4] = (uint8_t) (address >> 32);
  mac_reverse[3] = (uint8_t) (address >> 24);
  mac_reverse[2] = (uint8_t) (address >> 16);
  mac_reverse[1] = (uint8_t) (address >> 8);
  mac_reverse[0] = (uint8_t) (address >> 0);

  XiaomiAESVector vector{.key = {0},
                         .plaintext = {0},
                         .ciphertext = {0},
                         .authdata = {0x11},
                         .iv = {0},
                         .tag = {0},
                         .keysize = 16,
                         .authsize = 1,
                         .datasize = 0,
                         .tagsize = 4,
                         .ivsize = 12};

  vector.datasize = (raw.size() == 19) ? raw.size() - 12 : raw.size() - 18;
  int cipher_pos = (raw.size() == 19) ? 5 : 11;

  const uint8_t *v = raw.data();

  memcpy(vector.key, bindkey, vector.keysize);
  memcpy(vector.ciphertext, v + cipher_pos, vector.datasize);
  memcpy(vector.tag, v + raw.size() - vector.tagsize, vector.tagsize);
  memcpy(vector.iv, mac_reverse, 6);             // MAC address reverse
  memcpy(vector.iv + 6, v + 2, 3);               // sensor type (2) + packet id (1)
  memcpy(vector.iv + 9, v + raw.size() - 7, 3);  // payload counter

  mbedtls_ccm_context ctx;
  mbedtls_ccm_init(&ctx);

  int ret = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, vector.key, vector.keysize * 8);
  if (ret) {
    ESP_LOGVV(TAG, "decrypt_xiaomi_payload(): mbedtls_ccm_setkey() failed.");
    mbedtls_ccm_free(&ctx);
    return false;
  }

  ret = mbedtls_ccm_auth_decrypt(&ctx, vector.datasize, vector.iv, vector.ivsize, vector.authdata, vector.authsize,
                                 vector.ciphertext, vector.plaintext, vector.tag, vector.tagsize);
  if (ret) {
    uint8_t mac_address[6] = {0};
    memcpy(mac_address, mac_reverse + 5, 1);
    memcpy(mac_address + 1, mac_reverse + 4, 1);
    memcpy(mac_address + 2, mac_reverse + 3, 1);
    memcpy(mac_address + 3, mac_reverse + 2, 1);
    memcpy(mac_address + 4, mac_reverse + 1, 1);
    memcpy(mac_address + 5, mac_reverse, 1);
    ESP_LOGVV(TAG, "decrypt_xiaomi_payload(): authenticated decryption failed.");
    ESP_LOGVV(TAG, "  MAC address : %s", format_hex_pretty(mac_address, 6).c_str());
    ESP_LOGVV(TAG, "       Packet : %s", format_hex_pretty(raw.data(), raw.size()).c_str());
    ESP_LOGVV(TAG, "          Key : %s", format_hex_pretty(vector.key, vector.keysize).c_str());
    ESP_LOGVV(TAG, "           Iv : %s", format_hex_pretty(vector.iv, vector.ivsize).c_str());
    ESP_LOGVV(TAG, "       Cipher : %s", format_hex_pretty(vector.ciphertext, vector.datasize).c_str());
    ESP_LOGVV(TAG, "          Tag : %s", format_hex_pretty(vector.tag, vector.tagsize).c_str());
    mbedtls_ccm_free(&ctx);
    return false;
  }

  // replace encrypted payload with plaintext
  uint8_t *p = vector.plaintext;
  for (std::vector<uint8_t>::iterator it = raw.begin() + cipher_pos; it != raw.begin() + cipher_pos + vector.datasize;
       ++it) {
    *it = *(p++);
  }

  // clear encrypted flag
  raw[0] &= ~0x08;

  ESP_LOGVV(TAG, "decrypt_xiaomi_payload(): authenticated decryption passed.");
  ESP_LOGVV(TAG, "  Plaintext : %s, Packet : %d", format_hex_pretty(raw.data() + cipher_pos, vector.datasize).c_str(),
            static_cast<int>(raw[4]));

  mbedtls_ccm_free(&ctx);
  return true;
}

bool report_xiaomi_results(const optional<XiaomiParseResult> &result, const std::string &address) {
  if (!result.has_value()) {
    ESP_LOGVV(TAG, "report_xiaomi_results(): no results available.");
    return false;
  }

  ESP_LOGD(TAG, "Got Xiaomi %s (%s):", result->name.c_str(), address.c_str());

  if (result->event.has_value()) {
    ESP_LOGD(TAG, "  Status(Event): %d", *result->event);
    switch (*result->event) {
      case 0x00:
        ESP_LOGD(TAG, "  Status(Event): Normal");
        break;
      case 0x01:
        ESP_LOGD(TAG, "  Status(Event): Alert");
        break;
      case 0x02:
        ESP_LOGD(TAG, "  Status(Event): Fault");
        break;
      case 0x03:
        ESP_LOGD(TAG, "  Status(Event): Self-check");
        break;
      case 0x04:
        ESP_LOGD(TAG, "  Status(Event): Analog Alert");
        break;
      default:
        ESP_LOGD(TAG, "  Status(Event): Unkown(%d)", *result->event);
        break;
    }
  }
  if (result->battery_level.has_value()) {
    ESP_LOGD(TAG, "  Battery Level: %.0f%%", *result->battery_level);
  }

  return true;
}

}  // namespace xiaomi_smoke_ble
}  // namespace esphome

#endif
