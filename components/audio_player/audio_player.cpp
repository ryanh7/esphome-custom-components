#include "audio_player.h"
#include "esphome/core/log.h"
#include <cstring>

#include "AudioGeneratorMOD.h"
#include "AudioGeneratorWAV.h"
#include "AudioFileSourcePROGMEM.h"

namespace esphome {
namespace audio_player {

static const char *const TAG = "audio_player";

void AudioPlayerComponent::setup() {
  audioLogger = &logger_;

  if (pin_) {
    out_ = new AudioOutputI2SNoDACWithVolume(pin_->get_pin());
  } else {
    out_ = new AudioOutputI2SNoDACWithVolume();
  }

  out_->set_volume(base_volume_);
  generator_ = new AudioGeneratorWAV();
}

void AudioPlayerComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Audio Player:");
  if (pin_) {
    LOG_PIN("  Pin: ", this->pin_);
  }
  ESP_LOGCONFIG(TAG, "  Buffer Size: %d", buffer_size_);
  ESP_LOGCONFIG(TAG, "  Volume: %d%%", (int) (base_volume_ * 100));
}

void AudioPlayerComponent::loop() {
  if (!generator_->isRunning()) {
    return;
  }
  if (generator_->loop()) {
    return;
  }
  generator_->stop();
  if (buffer_ != NULL) {
    delete buffer_;
    buffer_ = NULL;
  }
  if (file_ != NULL) {
    delete file_;
    file_ = NULL;
  }
  for (auto *listener : listeners_) {
    listener->on_change(STATUS_IDLE);
  }
}

void AudioPlayerComponent::stop() {
  if (generator_->isRunning()) {
    generator_->stop();
  }
  if (buffer_ != NULL) {
    delete buffer_;
    buffer_ = NULL;
  }
  if (file_ != NULL) {
    delete file_;
    file_ = NULL;
  }
  for (auto *listener : listeners_) {
    listener->on_change(STATUS_IDLE);
  }
}

void AudioPlayerComponent::play(const std::string &url) {
  stop();

  ESP_LOGD(TAG, "play url %s", url.c_str());
  file_ = new AudioFileSourceHTTPStream(url.c_str());
  buffer_ = new AudioFileSourceBuffer(file_, buffer_size_);

  if (!generator_->begin(buffer_, out_)) {
    ESP_LOGE(TAG, "play failed");
    return;
  }

  for (auto *listener : listeners_) {
    listener->on_change(STATUS_PLAYING);
  }
}

size_t AudioLogger::write(const uint8_t *buffer, size_t size) {
  ESP_LOGD(TAG, "%s", buffer);
  return size;
}

size_t AudioLogger::write(uint8_t data) {
  printf("%c", data);
  return 1;
}

void AudioPlayerStatusTextSensor::setup() { this->publish_state("on"); }

void AudioPlayerStatusTextSensor::on_change(uint8_t status) {
  switch (status) {
    case STATUS_IDLE:
      this->publish_state("idle");
      break;
    case STATUS_PLAYING:
      this->publish_state("playing");
      break;
    default:
      this->publish_state("unkown");
      break;
  }
}

void AudioPlayerStatusTextSensor::dump_config() { ESP_LOGCONFIG(TAG, "Audio Player Status Text Sensor:"); }

}  // namespace audio_player
}  // namespace esphome
