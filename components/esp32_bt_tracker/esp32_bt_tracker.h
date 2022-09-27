#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "queue.h"

#ifdef USE_ESP32

#include <string>
#include <array>
#include <esp_gap_bt_api.h>
#include <esp_gattc_api.h>
#include <esp_bt_defs.h>

namespace esphome {
namespace esp32_bt_tracker {

class ESPBTDevice {
 public:
  void parse_scan_rst(const esp_bt_gap_cb_param_t::disc_res_param &param);

  std::string address_str() const;

  uint64_t address_uint64() const;

  const uint8_t *address() const { return address_; }

  int get_rssi() const { return rssi_; }
  const std::string &get_name() const { return this->name_; }

 protected:
  // void parse_eir_(uint8_t *eir);

  esp_bd_addr_t address_{
      0,
  };
  int8_t rssi_{-128};
  std::string name_{};
};

class ESP32BTTracker;

class ESPBTDeviceListener {
 public:
  virtual void on_scan_end() {}
  virtual bool parse_device(const ESPBTDevice &device) = 0;
  void set_parent(ESP32BTTracker *parent) { parent_ = parent; }

 protected:
  ESP32BTTracker *parent_{nullptr};
};

class ESP32BTTracker : public Component, public Nameable {
 public:
  void setup() override;
  void dump_config() override;

  void loop() override;
  uint32_t hash_base() override;
  void set_scan_duration(uint8_t scan_duration);

  void register_listener(ESPBTDeviceListener *listener) {
    listener->set_parent(this);
    this->listeners_.push_back(listener);
  }

 protected:
  bool bt_setup();
  void start_scan(bool first);
  static void IRAM_ATTR gap_event_handler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
  void gap_scan_start_complete();
  void gap_scan_stop_complete();
  void print_bt_device_info(ESPBTDevice &device);

  std::vector<uint64_t> already_discovered_;
  std::vector<ESPBTDeviceListener *> listeners_;

  SemaphoreHandle_t scan_end_lock_;
  uint8_t scan_duration_{0x10};

  Queue<ESPBTDevice> bt_devices_;
};

extern ESP32BTTracker *global_esp32_bt_tracker;

}  // namespace esp32_bt_tracker
}  // namespace esphome

#endif
