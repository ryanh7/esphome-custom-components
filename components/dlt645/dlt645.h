#pragma once

#include <utility>
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"

namespace esphome {
namespace dlt645 {

class DLT645Sensor : public sensor::Sensor {
 public:
  explicit DLT645Sensor(const std::string &type, uint32_t id, float format)
      : sensor::Sensor(), type_(type), data_id_(id), format_(format) {}
  void set_interval(uint32_t interval) { this->update_interval = interval; }
  bool is_timeout() { return last_update == 0 || micros() > last_update + update_interval; }
  float get_format() { return format_; }
  uint32_t get_id() { return data_id_; }
  const std::string &get_type() const { return this->type_; }
  uint32_t update_interval{10000000};
  uint32_t last_update{0};

 protected:
  std::string type_;
  uint32_t data_id_;
  float format_;
};

class DLT645Component : public Component, public remote_base::RemoteReceiverListener {
 public:
  void dump_config() override;
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void set_address(const std::string &address);
  void set_transmitter(remote_transmitter::RemoteTransmitterComponent *transmitter) {
    this->transmitter_ = transmitter;
  }
  void register_sensor(DLT645Sensor *sensor) { this->sensors_.push_back(sensor); }

 protected:
  bool on_receive(remote_base::RemoteReceiveData data) override;
  void send(std::vector<uint8_t> &address, uint8_t opcode, uint32_t data = 0xFFFFFFFF);
  uint8_t checksum_(std::vector<uint8_t> &data, int start, int length);
  void handle_response(std::vector<uint8_t> &data, int index);

  std::vector<uint8_t> address_;
  std::vector<uint8_t> rx_buffer_;
  uint32_t next_time_{0};
  remote_transmitter::RemoteTransmitterComponent *transmitter_;

  std::vector<DLT645Sensor *> sensors_;
};

}  // namespace dlt645
}  // namespace esphome
