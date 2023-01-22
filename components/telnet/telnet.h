#pragma once

#include <utility>

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/socket/socket.h"

namespace esphome {
namespace telnet {

class TelnetComponent : public uart::UARTDevice, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }
  void set_port(uint16_t port) { this->port_ = port; }

 protected:
  uint16_t port_{23};
  std::unique_ptr<socket::Socket> server_;
  std::unique_ptr<socket::Socket> client_;

  uint8_t buf_[1024];
};

}  // namespace telnet
}  // namespace esphome
