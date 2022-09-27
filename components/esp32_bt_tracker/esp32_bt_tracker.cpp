#include "esp32_bt_tracker.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP32

#include <nvs_flash.h>
#include <freertos/FreeRTOSConfig.h>
#include <esp_bt_main.h>
#include <esp_bt.h>
#include <freertos/task.h>
#include <esp_gap_bt_api.h>
#include <esp_bt_defs.h>
#include <esp_bt_device.h>

#undef TAG

namespace esphome {
namespace esp32_bt_tracker {

static const char *const TAG = "esp32_bt_tracker";

ESP32BTTracker *global_esp32_bt_tracker = nullptr;

uint64_t bt_addr_to_uint64(const esp_bd_addr_t address) {
  uint64_t u = 0;
  u |= uint64_t(address[0] & 0xFF) << 40;
  u |= uint64_t(address[1] & 0xFF) << 32;
  u |= uint64_t(address[2] & 0xFF) << 24;
  u |= uint64_t(address[3] & 0xFF) << 16;
  u |= uint64_t(address[4] & 0xFF) << 8;
  u |= uint64_t(address[5] & 0xFF) << 0;
  return u;
}

void ESP32BTTracker::setup() {
  global_esp32_bt_tracker = this;
  this->scan_end_lock_ = xSemaphoreCreateMutex();

  if (!bt_setup()) {
    this->mark_failed();
    return;
  }

  global_esp32_bt_tracker->start_scan(true);
}

void ESP32BTTracker::set_scan_duration(uint8_t scan_duration) {
  uint8_t duration = scan_duration / 1.28;
  if (duration < 0x01) {
    scan_duration_ = 0x01;
  } else if (duration > 0x30) {
    scan_duration_ = 0x30;
  } else {
    scan_duration_ = duration;
  }
}

uint32_t ESP32BTTracker::hash_base() { return 2158133466UL; }

bool ESP32BTTracker::bt_setup() {
  esp_err_t err;

  if (!btStarted()) {
    err = nvs_flash_init();
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "nvs_flash_init failed: %d", err);
      return false;
    }

    if (!btStart()) {
      ESP_LOGE(TAG, "btStart failed: %d", esp_bt_controller_get_status());
      return false;
    }

    err = esp_bluedroid_init();
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "esp_bluedroid_init failed: %d", err);
      return false;
    }
    err = esp_bluedroid_enable();
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "esp_bluedroid_enable failed: %d", err);
      return false;
    }
  }

  /* register GAP callback function */
  esp_bt_gap_register_callback(ESP32BTTracker::gap_event_handler);

  esp_bt_dev_set_device_name(this->get_name().c_str());
  esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_NONE);

  esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
  err = esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(uint8_t));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_bt_gap_set_security_param failed: %d", err);
    return false;
  }

  // BLE takes some time to be fully set up, 200ms should be more than enough
  delay(200);  // NOLINT

  return true;
}

void ESP32BTTracker::loop() {
  // 处理gap信息
  ESPBTDevice *device = this->bt_devices_.pop();
  while (device != nullptr) {
    bool found = false;
    for (auto *listener : this->listeners_)
      if (listener->parse_device(*device))
        found = true;
    // 没有监听器处理消息，打印蓝牙设备信息
    if (!found) {
      this->print_bt_device_info(*device);
    }
    delete device;
    device = this->bt_devices_.pop();
  }

  if (xSemaphoreTake(this->scan_end_lock_, 0L)) {
    xSemaphoreGive(this->scan_end_lock_);
    global_esp32_bt_tracker->start_scan(false);
  }
}

void ESP32BTTracker::start_scan(bool first) {
  if (!xSemaphoreTake(this->scan_end_lock_, 0L)) {
    ESP_LOGW(TAG, "Cannot start scan!");
    return;
  }

  ESP_LOGD(TAG, "Starting scan...");
  if (!first) {
    for (auto *listener : this->listeners_)
      listener->on_scan_end();
  }

  this->already_discovered_.clear();

  esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, this->scan_duration_, 0);

  this->set_timeout("scan", this->scan_duration_ * 3000, []() {
    ESP_LOGW(TAG, "ESP-IDF BT scan never terminated, rebooting to restore BT stack...");
    App.reboot();
  });
}

void ESP32BTTracker::gap_event_handler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
  switch (event) {
    case ESP_BT_GAP_DISC_RES_EVT: {
      ESPBTDevice *device = new ESPBTDevice();
      device->parse_scan_rst(param->disc_res);
      global_esp32_bt_tracker->bt_devices_.push(device);
    } break;
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
      if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
        global_esp32_bt_tracker->gap_scan_start_complete();
      } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
        global_esp32_bt_tracker->gap_scan_stop_complete();
        // 可以查询更多信息
      }
      break;
    default:
      break;
  }
}

void ESP32BTTracker::dump_config() {
  ESP_LOGCONFIG(TAG, "BT Tracker:");
  ESP_LOGCONFIG(TAG, "  Scan Duration: %u s", (uint8_t) (this->scan_duration_ * 1.28));
}

void ESP32BTTracker::gap_scan_start_complete() { ESP_LOGD(TAG, "scan started"); }

void ESP32BTTracker::gap_scan_stop_complete() {
  xSemaphoreGive(this->scan_end_lock_);
  ESP_LOGD(TAG, "scan stoped");
}

void ESP32BTTracker::print_bt_device_info(ESPBTDevice &device) {
  const uint64_t address = device.address_uint64();
  for (auto &disc : this->already_discovered_) {
    if (disc == address)
      return;
  }
  this->already_discovered_.push_back(address);

  ESP_LOGD(TAG, "Found device %s RSSI=%d", device.address_str().c_str(), device.get_rssi());

  if (!device.get_name().empty())
    ESP_LOGD(TAG, "  Name: '%s'", device.get_name().c_str());
}


void ESPBTDevice::parse_scan_rst(const esp_bt_gap_cb_param_t::disc_res_param &param) {
  esp_bt_gap_dev_prop_t *p;
  for (uint8_t i = 0; i < ESP_BD_ADDR_LEN; i++)
    this->address_[i] = param.bda[i];
  for (int i = 0; i < param.num_prop; i++) {
    p = param.prop + i;
    switch (p->type) {
      case ESP_BT_GAP_DEV_PROP_RSSI:
        this->rssi_ = *(int8_t *) (p->val);
        break;
      case ESP_BT_GAP_DEV_PROP_BDNAME:
        this->name_.assign((char *) p->val, p->len);
        break;
      /* case ESP_BT_GAP_DEV_PROP_EIR:
        parse_eir_((uint8_t *) p->val); */
      default:
        break;
    }
  }

#ifdef ESPHOME_LOG_HAS_VERY_VERBOSE
  ESP_LOGVV(TAG, "Parse Result:");

  ESP_LOGVV(TAG, "  Address: %02X:%02X:%02X:%02X:%02X:%02X (%s)", this->address_[0], this->address_[1],
            this->address_[2], this->address_[3], this->address_[4], this->address_[5], address_type);

  ESP_LOGVV(TAG, "  RSSI: %d", this->rssi_);
  ESP_LOGVV(TAG, "  Name: '%s'", this->name_.c_str());
#endif
}
/* void ESPBTDevice::parse_eir_(uint8_t *eir) {
  uint8_t len = 0;
  uint16_t *uuid16 = (uint16_t *) esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_INCMPL_16BITS_UUID, &len);
  for (int i = 0; i < len; i++) {
    this->service_uuids_.push_back(ESPBTUUID::from_uint16(uuid16[i]));
  }

  len = 0;
  uint32_t *uuid32 = (uint32_t *) esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_INCMPL_32BITS_UUID, &len);
  for (int i = 0; i < len; i++) {
    this->service_uuids_.push_back(ESPBTUUID::from_uint32(uuid32[i]));
  }

  len = 0;
  uint8_t *uuid = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_INCMPL_128BITS_UUID, &len);
  for (int i = 0; i < len; i++) {
    this->service_uuids_.push_back(ESPBTUUID::from_raw(&uuid[i * ESP_UUID_LEN_128]));
  }

  len = 0;
  uint8_t *tx_power = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_TX_POWER_LEVEL, &len);
  for (int i = 0; i < len; i++) {
    this->tx_powers_.push_back(tx_power[i]);
  }

  len = 0;
  uint8_t *manufacturer_data = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_MANU_SPECIFIC, &len);
  if (len > 0) {
    ServiceData data{};
    data.uuid = ESPBTUUID::from_uint16(*reinterpret_cast<const uint16_t *>(manufacturer_data));
    data.data.assign(manufacturer_data + 2UL, manufacturer_data + len);
    this->manufacturer_datas_.push_back(data);
  }
} */

std::string ESPBTDevice::address_str() const {
  char mac[24];
  snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", this->address_[0], this->address_[1], this->address_[2],
           this->address_[3], this->address_[4], this->address_[5]);
  return mac;
}
uint64_t ESPBTDevice::address_uint64() const { return bt_addr_to_uint64(this->address_); }

}  // namespace esp32_bt_tracker
}  // namespace esphome

#endif
