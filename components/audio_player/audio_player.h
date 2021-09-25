#pragma once

#include <utility>
#include <SD.h>
#ifdef ESP32
#include "SPIFFS.h"
#endif

#include "esphome/core/component.h"
#include "esphome/core/esphal.h"
#include "esphome/core/automation.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include "AudioLogger.h"

#include "AudioOutputI2SNoDAC.h"
#include "AudioGenerator.h"
#include "AudioFileSource.h"
#include "AudioFileSourceBuffer.h"

#include "AudioFileSourceHTTPStream.h"

namespace esphome {
namespace audio_player {

enum status_type { STATUS_IDLE, STATUS_PLAYING };

class AudioLogger : public Print {
 public:
  size_t write(const uint8_t *buffer, size_t size) override;
  size_t write(uint8_t data) override;
};

class AudioOutputI2SNoDACWithVolume : public AudioOutputI2SNoDAC {
 public:
  AudioOutputI2SNoDACWithVolume(int port = 0) : AudioOutputI2SNoDAC(port) {}
  virtual bool ConsumeSample(int16_t sample[2]) override {
    int16_t sample_adjust[2];
    sample_adjust[0] = (int16_t) (sample[0] * volume_);
    sample_adjust[1] = (int16_t) (sample[1] * volume_);
    return AudioOutputI2SNoDAC::ConsumeSample(sample_adjust);
  }
  void set_volume(float volume) { volume_ = volume; }

 protected:
  float volume_{1.0f};
};

class StatusListener {
 public:
  virtual void on_change(uint8_t status) = 0;
};

class AudioPlayerComponent : public Component {
 public:
  AudioPlayerComponent() {}
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void register_listener(StatusListener *listener) { this->listeners_.push_back(listener); }

  void set_pin(GPIOPin *pin) { pin_ = pin; }
  void set_buffer_size(uint32_t size) { buffer_size_ = size; }
  void set_volume_percent(uint16_t percent) { base_volume_ = (float) percent / 100; }

  void play(const std::string &url);
  void stop();

 protected:
  GPIOPin *pin_{NULL};
  uint32_t buffer_size_{1024};
  float base_volume_{1.0f};
  AudioOutputI2SNoDACWithVolume *out_;
  AudioGenerator *generator_{NULL};
  AudioFileSource *file_{NULL};
  AudioFileSourceBuffer *buffer_;
  AudioLogger logger_;

  std::vector<StatusListener *> listeners_;
};

template<typename... Ts> class PlayAudioAction : public Action<Ts...> {
 public:
  PlayAudioAction(AudioPlayerComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, url)

  void play(Ts... x) { this->parent_->play(url_.value(x...)); }

 protected:
  AudioPlayerComponent *parent_;
};

template<typename... Ts> class StopAudioAction : public Action<Ts...> {
 public:
  StopAudioAction(AudioPlayerComponent *parent) : parent_(parent) {}

  void play(Ts... x) { this->parent_->stop(); }

 protected:
  AudioPlayerComponent *parent_;
};

class AudioPlayerStatusTextSensor : public text_sensor::TextSensor, public StatusListener, public Component {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  virtual void on_change(uint8_t status) override;

 protected:
  bool hide_timestamp_{false};
};

}  // namespace audio_player
}  // namespace esphome
