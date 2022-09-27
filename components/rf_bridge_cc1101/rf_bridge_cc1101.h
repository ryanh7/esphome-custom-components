#pragma once

#include <utility>

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/spi/spi.h"
#include "esphome/core/automation.h"

#include "remote_transmitter.h"
#include "remote_receiver.h"

namespace esphome {
namespace rf_bridge_cc1101 {

enum rf_bridge_mode_type { MODE_RECEIVER, MODE_TRANSMITTER };

class RFBridgeComponent : public Component,
                          public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                                                spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_4MHZ> {
 public:
  RFBridgeComponent(InternalGPIOPin *pin);
  RFBridgeComponent() {}
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void receiver_mode();
  void transmitter_mode();
  void set_mode(uint8_t mode) { mode_ = mode; }
  void set_frequency_mhz(float mhz) { mhz_ = mhz; }

  void transmit(const std::string &code, uint8_t protocol, uint32_t send_times, uint32_t send_wait);

  void register_listener(remote_base::RemoteReceiverListener *listener);
  void register_dumper(remote_base::RemoteReceiverDumperBase *dumper);

 protected:
  void cc1101_setup();
  void write_reg(uint8_t addr, uint8_t value);
  void write_burst_reg(uint8_t addr, uint8_t *value, size_t lenght);
  void strobe(uint8_t strobe);
  uint8_t read_reg(uint8_t addr);
  void set_mhz(float mhz);
  void set_pa(int pa);
  void calibrate();
  void set_io_mode();
  void set_modulation(uint8_t modulation);
  void split_MDMCFG2();
  void reset();

  InternalGPIOPin *pin_;
  uint8_t mode_{MODE_RECEIVER};

  float mhz_{433.92};
  int last_pa_, pa_{12};
  uint8_t modulation_{2};
  uint8_t m2DCOFF, m2MANCH, m2MODFM, m2SYNCM, frend0, m4RxBw;
  uint8_t clb1[2]{24, 28};
  uint8_t clb2[2]{31, 38};
  uint8_t clb3[2]{65, 76};
  uint8_t clb4[2]{77, 79};
  uint8_t PA_TABLE[8]{0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  RemoteTransmitter *transmitter_{NULL};
  RemoteReceiver *receiver_{NULL};
};

template<typename... Ts> class RFBridgeReceiverModeAction : public Action<Ts...> {
 public:
  RFBridgeReceiverModeAction(RFBridgeComponent *parent) : parent_(parent) {}

  void play(Ts... x) { this->parent_->receiver_mode(); }

 protected:
  RFBridgeComponent *parent_;
};

template<typename... Ts> class RFBridgeTransmitterModeAction : public Action<Ts...> {
 public:
  RFBridgeTransmitterModeAction(RFBridgeComponent *parent) : parent_(parent) {}

  void play(Ts... x) { this->parent_->transmitter_mode(); }

 protected:
  RFBridgeComponent *parent_;
};

template<typename... Ts> class RFBridgeTransmitAction : public Action<Ts...> {
 public:
  RFBridgeTransmitAction(RFBridgeComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, code)
  TEMPLATABLE_VALUE(uint8_t, protocol)
  TEMPLATABLE_VALUE(uint32_t, send_times)
  TEMPLATABLE_VALUE(uint32_t, send_wait)

  void play(Ts... x) {
    this->parent_->transmit(code_.value(x...), protocol_.value(x...), send_times_.value_or(x..., 1),
                            send_wait_.value_or(x..., 0));
  }

 protected:
  RFBridgeComponent *parent_;
};

}  // namespace rf_bridge_cc1101
}  // namespace esphome
