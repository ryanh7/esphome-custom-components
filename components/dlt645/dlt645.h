#pragma once

#include <utility>
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"

namespace esphome {
namespace dlt645 {

class DLT645Sensor : public sensor::Sensor {
 public:
  void set_interval(uint32_t interval) { this->update_interval = interval; }
  bool is_timeout() { return last_update == 0 || micros() > last_update + update_interval; }
  uint32_t update_interval{10000000};
  uint32_t last_update{0};
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
  void set_power_sensor(DLT645Sensor *sensor) { power_sensor_ = sensor; }
  void set_energy_sensor(DLT645Sensor *sensor) { energy_sensor_ = sensor; }
  void set_energy_a_sensor(DLT645Sensor *sensor) { energy_sensor_a_ = sensor; }
  void set_energy_b_sensor(DLT645Sensor *sensor) { energy_sensor_b_ = sensor; }
  void set_energy_c_sensor(DLT645Sensor *sensor) { energy_sensor_c_ = sensor; }

 protected:
  bool on_receive(remote_base::RemoteReceiveData data) override;
  void send(std::vector<uint8_t> &address, uint8_t opcode, std::vector<uint8_t> *data, uint16_t wait_ms = 0);
  uint8_t checksum_(std::vector<uint8_t> &data, int start, int length);
  void handle_response(std::vector<uint8_t> &data, int index);

  std::vector<uint8_t> address_;
  std::vector<uint8_t> rx_buffer_;
  uint32_t next_time_{0};
  remote_transmitter::RemoteTransmitterComponent *transmitter_;

  DLT645Sensor *power_sensor_{nullptr};
  DLT645Sensor *energy_sensor_{nullptr};
  DLT645Sensor *energy_sensor_a_{nullptr};
  DLT645Sensor *energy_sensor_b_{nullptr};
  DLT645Sensor *energy_sensor_c_{nullptr};
};

}  // namespace dlt645
}  // namespace esphome
