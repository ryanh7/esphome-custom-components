#pragma once
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/automation.h"
#include "AudioOutputI2SNoDAC.h"
#include "AudioGenerator.h"
#include "AudioFileSource.h"
#include "AudioFileSourceBuffer.h"
#include "esphome/components/media_player/media_player.h"

namespace esphome {
namespace audio_player {

enum status_type {
  STATUS_IDLE = 0,
  STATUS_PLAYING,
  STATUS_PAUSE,
  STATUS_MUTE,
  STATUS_UNMUTE,
  STATUS_VOLUME_ZERO = 100,
  STATUS_VOLUME_MAX = 200
};

class Dumpable {
 public:
  virtual void dump_config() = 0;
};

class Volume {
 public:
  virtual void set_volume(float volume) = 0;
};

template<typename T> class VolumeOutput : public T, public Volume {
 public:
  using T::T;
  virtual bool ConsumeSample(int16_t sample[2]) override {
    int16_t sample_adjust[2];
    double sample_rescale = (double) sample[0] * (double) volume_;
    sample_rescale = (sample_rescale < -32768) ? -32768 : sample_rescale;
    sample_rescale = (sample_rescale > 32767) ? 32767 : sample_rescale;
    sample_adjust[0] = (int16_t) (sample_rescale);
    sample_rescale = (double) sample[1] * (double) volume_;
    sample_rescale = (sample_rescale < -32768) ? -32768 : sample_rescale;
    sample_rescale = (sample_rescale > 32767) ? 32767 : sample_rescale;
    sample_adjust[1] = (int16_t) (sample_rescale);
    return T::ConsumeSample(sample_adjust);
  }
  virtual void set_volume(float volume) { volume_ = volume; }

 protected:
  float volume_{1.0f};
};

class PlayerOutputI2S : public VolumeOutput<AudioOutputI2S>, public Dumpable {
 public:
  void set_pins(InternalGPIOPin *bclk, InternalGPIOPin *wclk, InternalGPIOPin *dout);
  virtual void dump_config() override;

 protected:
  InternalGPIOPin *bclk_;
  InternalGPIOPin *wclk_;
  InternalGPIOPin *dout_;
};

class PlayerOutputI2SNoDAC : public VolumeOutput<AudioOutputI2SNoDAC>, public Dumpable {
 public:
  virtual void dump_config() override;
};


class AudioMediaPlayer : public Component, public media_player::MediaPlayer {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  media_player::MediaPlayerTraits get_traits() override;
  bool is_muted() const override { return this->muted_; }

  void play(const std::string &url);
  void stop();

  void set_output(AudioOutput *out) { this->out_ = out; }
  void set_volume_controller(Volume *volume_contorller) { this->volume_contorller_ = volume_contorller; }
  void set_ext_info(Dumpable *info) { this->ext_info_ = info; }
  void set_buffer_size(uint32_t size) { this->buffer_size_ = size; }
  void set_base_volume(float base_volume) { this->base_volume_ = base_volume; }
  void set_volume(float volume);

  float get_setup_priority() const override { return setup_priority::LATE; }

 protected:
  void control(const media_player::MediaPlayerCall &call) override;
  void mute_();
  void unmute_();
  bool pause_{false};
  bool muted_{false};

  float base_volume_{1.0f};
  uint32_t buffer_size_{1024};

  AudioGenerator *generator_{NULL};
  AudioFileSource *file_{NULL};
  AudioFileSourceBuffer *buffer_{NULL};
  AudioOutput *out_{NULL};
  Volume *volume_contorller_{NULL};
  Dumpable *ext_info_{NULL};
};

}  // namespace audio_player
}  // namespace esphome
