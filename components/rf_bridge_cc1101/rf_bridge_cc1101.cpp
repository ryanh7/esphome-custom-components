#include "rf_bridge_cc1101.h"
#include "esphome/core/log.h"
#include "cc1101_def.h"
#include "esphome/components/remote_base/rc_switch_protocol.h"
#include <cstring>

namespace esphome {
namespace rf_bridge_cc1101 {

//  -30   -20   -15   -10   0     5     7     10
uint8_t PA_TABLE_315[8]{
    0x12, 0x0D, 0x1C, 0x34, 0x51, 0x85, 0xCB, 0xC2,
};  // 300 - 348
uint8_t PA_TABLE_433[8]{
    0x12, 0x0E, 0x1D, 0x34, 0x60, 0x84, 0xC8, 0xC0,
};  // 387 - 464
//  -30   -20   -15   -10   -6    0     5     7     10    12
uint8_t PA_TABLE_868[10]{
    0x03, 0x17, 0x1D, 0x26, 0x37, 0x50, 0x86, 0xCD, 0xC5, 0xC0,
};  // 779 - 899.99
//  -30   -20   -15   -10   -6    0     5     7     10    11
uint8_t PA_TABLE_915[10]{
    0x03, 0x0E, 0x1E, 0x27, 0x38, 0x8E, 0x84, 0xCC, 0xC3, 0xC0,
};  // 900 - 928

static const char *const TAG = "rf_bridge_cc1101";

RFBridgeComponent::RFBridgeComponent(InternalGPIOPin *pin) : pin_(pin) {
  if (!pin) {
    return;
  }
  transmitter_ = new RemoteTransmitter(pin);
  receiver_ = new RemoteReceiver(pin);
}

void RFBridgeComponent::setup() {
  if (transmitter_) {
    transmitter_->set_carrier_duty_percent(100);
    transmitter_->setup();
  }

  if (receiver_) {
#ifdef USE_ESP32
    receiver_->set_buffer_size(10240);
#else
    receiver_->set_buffer_size(1024);
#endif
    receiver_->set_filter_us(50);
    receiver_->set_idle_us(10000);
    receiver_->set_tolerance(25);
    receiver_->setup();
  }

  cc1101_setup();

  if (mode_ == MODE_TRANSMITTER) {
    transmitter_mode();
  } else {
    receiver_mode();
  }
}

const char *status_to_str(int status) {
  switch (status) {
    case 1:
      return "IDLE";
    case 13:
    case 14:
    case 15:
      return "RX";
    case 19:
    case 20:
      return "TX";
    default:
      return "UNKOWN";
  }
}

void RFBridgeComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "RF_Bridge_CC1101:");
  ESP_LOGCONFIG(TAG, "  Frequency: %3.2f MHz", mhz_);
  int status = read_reg(CC1101_MARCSTATE);
  ESP_LOGCONFIG(TAG, "  Status: %d (%s)", status, status_to_str(status));
  int rssi = read_reg(CC1101_RSSI);
  ESP_LOGCONFIG(TAG, "  RSSI: %d dB", rssi >= 128 ? (rssi - 256) / 2 - 74 : (rssi / 2) - 74);
}

void RFBridgeComponent::loop() {
  if (receiver_) {
    receiver_->loop();
  }
}

void RFBridgeComponent::register_listener(remote_base::RemoteReceiverListener *listener) {
  if (receiver_) {
    receiver_->register_listener(listener);
  }
}

void RFBridgeComponent::register_dumper(remote_base::RemoteReceiverDumperBase *dumper) {
  if (receiver_) {
    receiver_->register_dumper(dumper);
  }
}

void RFBridgeComponent::transmit(const std::string &code, uint8_t protocol, uint32_t send_times, uint32_t send_wait) {
  if (!transmitter_) {
    ESP_LOGE(TAG, "no transmitter");
    return;
  }
  if (protocol >= sizeof(remote_base::RC_SWITCH_PROTOCOLS)) {
    ESP_LOGE(TAG, "protocal should less then %d.", sizeof(remote_base::RC_SWITCH_PROTOCOLS));
    return;
  }
  transmitter_mode();
  auto call = transmitter_->transmit();

  uint64_t the_code = remote_base::decode_binary_string(code);
  uint8_t nbits = code.size();

  auto proto = remote_base::RC_SWITCH_PROTOCOLS[protocol];
  proto.transmit(call.get_data(), the_code, nbits);

  call.set_send_times(send_times);
  call.set_send_wait(send_wait);
  call.perform();
  receiver_mode();
}

void RFBridgeComponent::cc1101_setup() {
  spi_setup();

  reset();

  write_reg(CC1101_FSCTRL1, 0x06);

  set_io_mode();
  set_mhz(mhz_);

  write_reg(CC1101_MDMCFG1, 0x02);
  write_reg(CC1101_MDMCFG0, 0xF8);
  write_reg(CC1101_CHANNR, 0x00);
  write_reg(CC1101_DEVIATN, 0x47);
  write_reg(CC1101_FREND1, 0x56);
  write_reg(CC1101_MCSM0, 0x18);
  write_reg(CC1101_FOCCFG, 0x16);
  write_reg(CC1101_BSCFG, 0x1C);
  write_reg(CC1101_AGCCTRL2, 0xC7);
  write_reg(CC1101_AGCCTRL1, 0x00);
  write_reg(CC1101_AGCCTRL0, 0xB2);  // 灵敏度
  write_reg(CC1101_FSCAL3, 0xE9);
  write_reg(CC1101_FSCAL2, 0x2A);
  write_reg(CC1101_FSCAL1, 0x00);
  write_reg(CC1101_FSCAL0, 0x1F);
  write_reg(CC1101_FSTEST, 0x59);
  write_reg(CC1101_TEST2, 0x81);
  write_reg(CC1101_TEST1, 0x35);
  write_reg(CC1101_TEST0, 0x09);
  write_reg(CC1101_PKTCTRL1, 0x04);
  write_reg(CC1101_ADDR, 0x00);
  write_reg(CC1101_PKTLEN, 0x00);
}

void RFBridgeComponent::reset() {
  cs_->digital_write(false);
  delay(1);
  cs_->digital_write(true);
  delay(1);
  enable();
  delay(1);
  write_byte(CC1101_SRES);
  delay(1);
  disable();
}

void RFBridgeComponent::write_reg(uint8_t addr, uint8_t value) {
  enable();
  write_byte(addr);
  write_byte(value);
  disable();
}

void RFBridgeComponent::write_burst_reg(uint8_t addr, uint8_t *value, size_t length) {
  enable();
  write_byte(addr | WRITE_BURST);
  write_array(value, length);
  disable();
}

void RFBridgeComponent::strobe(uint8_t strobe) {
  enable();
  write_byte(strobe);
  disable();
}

uint8_t RFBridgeComponent::read_reg(uint8_t addr) {
  enable();
  write_byte(addr | READ_BURST);
  uint8_t ret = read_byte();
  disable();
  return ret;
}

void RFBridgeComponent::receiver_mode() {
  strobe(CC1101_SIDLE);
  if (receiver_) {
    receiver_->enable();
  }
  strobe(CC1101_SRX);
}

void RFBridgeComponent::transmitter_mode() {
  strobe(CC1101_SIDLE);
  strobe(CC1101_STX);
  if (receiver_) {
    receiver_->diable();
  }
  if (transmitter_) {
    transmitter_->enable();
  }
}

void RFBridgeComponent::set_mhz(float mhz) {
  byte freq2 = 0;
  byte freq1 = 0;
  byte freq0 = 0;

  mhz_ = mhz;

  for (bool i = 0; i == 0;) {
    if (mhz >= 26) {
      mhz -= 26;
      freq2 += 1;
    } else if (mhz >= 0.1015625) {
      mhz -= 0.1015625;
      freq1 += 1;
    } else if (mhz >= 0.00039675) {
      mhz -= 0.00039675;
      freq0 += 1;
    } else {
      i = 1;
    }
  }
  if (freq0 > 255) {
    freq1 += 1;
    freq0 -= 256;
  }

  write_reg(CC1101_FREQ2, freq2);
  write_reg(CC1101_FREQ1, freq1);
  write_reg(CC1101_FREQ0, freq0);

  calibrate();
}

void RFBridgeComponent::calibrate(void) {
  if (mhz_ >= 300 && mhz_ <= 348) {
    write_reg(CC1101_FSCTRL0, map(mhz_, 300, 348, clb1[0], clb1[1]));
    if (mhz_ < 322.88) {
      write_reg(CC1101_TEST0, 0x0B);
    } else {
      write_reg(CC1101_TEST0, 0x09);
      int s = read_reg(CC1101_FSCAL2);
      if (s < 32) {
        write_reg(CC1101_FSCAL2, s + 32);
      }
      if (last_pa_ != 1) {
        set_pa(pa_);
      }
    }
  } else if (mhz_ >= 378 && mhz_ <= 464) {
    write_reg(CC1101_FSCTRL0, map(mhz_, 378, 464, clb2[0], clb2[1]));
    if (mhz_ < 430.5) {
      write_reg(CC1101_TEST0, 0x0B);
    } else {
      write_reg(CC1101_TEST0, 0x09);
      int s = read_reg(CC1101_FSCAL2);
      if (s < 32) {
        write_reg(CC1101_FSCAL2, s + 32);
      }
      if (last_pa_ != 2) {
        set_pa(pa_);
      }
    }
  } else if (mhz_ >= 779 && mhz_ <= 899.99) {
    write_reg(CC1101_FSCTRL0, map(mhz_, 779, 899, clb3[0], clb3[1]));
    if (mhz_ < 861) {
      write_reg(CC1101_TEST0, 0x0B);
    } else {
      write_reg(CC1101_TEST0, 0x09);
      int s = read_reg(CC1101_FSCAL2);
      if (s < 32) {
        write_reg(CC1101_FSCAL2, s + 32);
      }
      if (last_pa_ != 3) {
        set_pa(pa_);
      }
    }
  } else if (mhz_ >= 900 && mhz_ <= 928) {
    write_reg(CC1101_FSCTRL0, map(mhz_, 900, 928, clb4[0], clb4[1]));
    write_reg(CC1101_TEST0, 0x09);
    int s = read_reg(CC1101_FSCAL2);
    if (s < 32) {
      write_reg(CC1101_FSCAL2, s + 32);
    }
    if (last_pa_ != 4) {
      set_pa(pa_);
    }
  }
}

void RFBridgeComponent::set_pa(int p) {
  int a = 0xC0;
  pa_ = p;

  if (mhz_ >= 300 && mhz_ <= 348) {
    if (pa_ <= -30) {
      a = PA_TABLE_315[0];
    } else if (pa_ > -30 && pa_ <= -20) {
      a = PA_TABLE_315[1];
    } else if (pa_ > -20 && pa_ <= -15) {
      a = PA_TABLE_315[2];
    } else if (pa_ > -15 && pa_ <= -10) {
      a = PA_TABLE_315[3];
    } else if (pa_ > -10 && pa_ <= 0) {
      a = PA_TABLE_315[4];
    } else if (pa_ > 0 && pa_ <= 5) {
      a = PA_TABLE_315[5];
    } else if (pa_ > 5 && pa_ <= 7) {
      a = PA_TABLE_315[6];
    } else if (pa_ > 7) {
      a = PA_TABLE_315[7];
    }
    last_pa_ = 1;
  } else if (mhz_ >= 378 && mhz_ <= 464) {
    if (pa_ <= -30) {
      a = PA_TABLE_433[0];
    } else if (pa_ > -30 && pa_ <= -20) {
      a = PA_TABLE_433[1];
    } else if (pa_ > -20 && pa_ <= -15) {
      a = PA_TABLE_433[2];
    } else if (pa_ > -15 && pa_ <= -10) {
      a = PA_TABLE_433[3];
    } else if (pa_ > -10 && pa_ <= 0) {
      a = PA_TABLE_433[4];
    } else if (pa_ > 0 && pa_ <= 5) {
      a = PA_TABLE_433[5];
    } else if (pa_ > 5 && pa_ <= 7) {
      a = PA_TABLE_433[6];
    } else if (pa_ > 7) {
      a = PA_TABLE_433[7];
    }
    last_pa_ = 2;
  } else if (mhz_ >= 779 && mhz_ <= 899.99) {
    if (pa_ <= -30) {
      a = PA_TABLE_868[0];
    } else if (pa_ > -30 && pa_ <= -20) {
      a = PA_TABLE_868[1];
    } else if (pa_ > -20 && pa_ <= -15) {
      a = PA_TABLE_868[2];
    } else if (pa_ > -15 && pa_ <= -10) {
      a = PA_TABLE_868[3];
    } else if (pa_ > -10 && pa_ <= -6) {
      a = PA_TABLE_868[4];
    } else if (pa_ > -6 && pa_ <= 0) {
      a = PA_TABLE_868[5];
    } else if (pa_ > 0 && pa_ <= 5) {
      a = PA_TABLE_868[6];
    } else if (pa_ > 5 && pa_ <= 7) {
      a = PA_TABLE_868[7];
    } else if (pa_ > 7 && pa_ <= 10) {
      a = PA_TABLE_868[8];
    } else if (pa_ > 10) {
      a = PA_TABLE_868[9];
    }
    last_pa_ = 3;
  } else if (mhz_ >= 900 && mhz_ <= 928) {
    if (pa_ <= -30) {
      a = PA_TABLE_915[0];
    } else if (pa_ > -30 && pa_ <= -20) {
      a = PA_TABLE_915[1];
    } else if (pa_ > -20 && pa_ <= -15) {
      a = PA_TABLE_915[2];
    } else if (pa_ > -15 && pa_ <= -10) {
      a = PA_TABLE_915[3];
    } else if (pa_ > -10 && pa_ <= -6) {
      a = PA_TABLE_915[4];
    } else if (pa_ > -6 && pa_ <= 0) {
      a = PA_TABLE_915[5];
    } else if (pa_ > 0 && pa_ <= 5) {
      a = PA_TABLE_915[6];
    } else if (pa_ > 5 && pa_ <= 7) {
      a = PA_TABLE_915[7];
    } else if (pa_ > 7 && pa_ <= 10) {
      a = PA_TABLE_915[8];
    } else if (pa_ > 10) {
      a = PA_TABLE_915[9];
    }
    last_pa_ = 4;
  }
  if (modulation_ == 2) {
    PA_TABLE[0] = 0;
    PA_TABLE[1] = a;
  } else {
    PA_TABLE[0] = a;
    PA_TABLE[1] = 0;
  }
  write_burst_reg(CC1101_PATABLE, PA_TABLE, 8);
}

void RFBridgeComponent::set_io_mode() {
  write_reg(CC1101_IOCFG2, 0x0D);
  write_reg(CC1101_IOCFG0, 0x0D);
  write_reg(CC1101_PKTCTRL0, 0x32);
  write_reg(CC1101_MDMCFG3, 0x93);
  write_reg(CC1101_MDMCFG4, 7 + m4RxBw);

  set_modulation(modulation_);
}

void RFBridgeComponent::set_modulation(uint8_t modulation) {
  if (modulation > 4) {
    modulation = 4;
  }
  modulation_ = modulation;
  split_MDMCFG2();
  switch (modulation) {
    case 0:
      m2MODFM = 0x00;
      frend0 = 0x10;
      break;  // 2-FSK
    case 1:
      m2MODFM = 0x10;
      frend0 = 0x10;
      break;  // GFSK
    case 2:
      m2MODFM = 0x30;
      frend0 = 0x11;
      break;  // ASK
    case 3:
      m2MODFM = 0x40;
      frend0 = 0x10;
      break;  // 4-FSK
    case 4:
      m2MODFM = 0x70;
      frend0 = 0x10;
      break;  // MSK
  }
  write_reg(CC1101_MDMCFG2, m2DCOFF + m2MODFM + m2MANCH + m2SYNCM);
  write_reg(CC1101_FREND0, frend0);
  set_pa(pa_);
}

void RFBridgeComponent::split_MDMCFG2() {
  int calc = read_reg(18);
  m2DCOFF = 0;
  m2MODFM = 0;
  m2MANCH = 0;
  m2SYNCM = 0;
  for (bool i = 0; i == 0;) {
    if (calc >= 128) {
      calc -= 128;
      m2DCOFF += 128;
    } else if (calc >= 16) {
      calc -= 16;
      m2MODFM += 16;
    } else if (calc >= 8) {
      calc -= 8;
      m2MANCH += 8;
    } else {
      m2SYNCM = calc;
      i = 1;
    }
  }
}

}  // namespace rf_bridge_cc1101
}  // namespace esphome
