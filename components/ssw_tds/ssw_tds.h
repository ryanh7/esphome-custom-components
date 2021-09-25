#pragma once

#include <utility>

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace ssw_tds {

static const uint8_t URAT_QUERY_COMMAND[] ={0xff, 0x06, 0x01, 0x05};
static const uint8_t URAT_FEEDBACK_START = 0xfe;


class SSWTDSComponent : public uart::UARTDevice, public PollingComponent {
 public:
  void dump_config() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  
  void set_source_tds_sensor(sensor::Sensor *source_tds_sensor) { source_tds_sensor_ = source_tds_sensor; }
  void set_clean_tds_sensor(sensor::Sensor *clean_tds_sensor) { clean_tds_sensor_ = clean_tds_sensor; }
  void set_temperature_sensor(sensor::Sensor *temperature_sensor) { temperature_sensor_ = temperature_sensor; }

 protected:
  void query_();
  int parse_(uint8_t byte);
  int checksum_();

  std::vector<uint8_t> rx_buffer_;
  uint32_t last_byte_{0};

  sensor::Sensor *source_tds_sensor_{nullptr};
  sensor::Sensor *clean_tds_sensor_{nullptr};
  sensor::Sensor *temperature_sensor_{nullptr};
};


}  // namespace ssw_tds
}  // namespace esphome
