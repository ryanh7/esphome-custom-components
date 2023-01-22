#include "telnet.h"
#include "esphome/core/log.h"
#include <cstring>

namespace esphome {
namespace telnet {

static const char *const TAG = "telnet";

void TelnetComponent::setup() {
  server_ = socket::socket_ip(SOCK_STREAM, 0);
  if (server_ == nullptr) {
    ESP_LOGW(TAG, "Could not create socket.");
    this->mark_failed();
    return;
  }
  int enable = 1;
  int err = server_->setsockopt(SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  if (err != 0) {
    ESP_LOGW(TAG, "Socket unable to set reuseaddr: errno %d", err);
    // we can still continue
  }
  err = server_->setblocking(false);
  if (err != 0) {
    ESP_LOGW(TAG, "Socket unable to set nonblocking mode: errno %d", err);
    this->mark_failed();
    return;
  }

  struct sockaddr_storage server;

  socklen_t sl = socket::set_sockaddr_any((struct sockaddr *) &server, sizeof(server), htons(this->port_));
  if (sl == 0) {
    ESP_LOGW(TAG, "Socket unable to set sockaddr: errno %d", errno);
    this->mark_failed();
    return;
  }

  err = server_->bind((struct sockaddr *) &server, sizeof(server));
  if (err != 0) {
    ESP_LOGW(TAG, "Socket unable to bind: errno %d", errno);
    this->mark_failed();
    return;
  }

  err = server_->listen(4);
  if (err != 0) {
    ESP_LOGW(TAG, "Socket unable to listen: errno %d", errno);
    this->mark_failed();
    return;
  }
}

void TelnetComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Telnet:");
  ESP_LOGCONFIG(TAG, "  Port: %d", this->port_);
  if (client_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Remote: %s", this->client_->getpeername().c_str());
  }
}

void TelnetComponent::loop() {
  if (client_ == nullptr) {
    struct sockaddr_storage source_addr;
    socklen_t addr_len = sizeof(source_addr);
    client_ = server_->accept((struct sockaddr *) &source_addr, &addr_len);
    if (client_ == nullptr)
      return;
    ESP_LOGI(TAG, "Accept connection from %s...", this->client_->getpeername().c_str());

    int err = client_->setblocking(false);
    if (err != 0) {
      ESP_LOGW(TAG, "Setting nonblocking failed with errno %d", errno);
    }

    int enable = 1;
    err = client_->setsockopt(IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));
    if (err != 0) {
      ESP_LOGW(TAG, "Socket could not enable tcp nodelay, errno: %d", errno);
    }
  }

  while (true) {
    int write = this->available();
    if (write > 0) {
      write = std::min(write, (int) sizeof(buf_));
      this->read_array(buf_, write);
      client_->write(buf_, write);
    }

    ssize_t read = this->client_->read(buf_, sizeof(buf_));
    if (read == -1) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        ESP_LOGW(TAG, "Error receiving data, errno: %d", errno);
        this->client_->close();
        this->client_ = nullptr;
        return;
      }
    } else if (read == 0) {
      ESP_LOGI(TAG, "Remote end closed connection");
      this->client_->close();
      this->client_ = nullptr;
      return;
    } else {
      this->write_array(buf_, read);
      this->flush();
    }

    if (write < 1 && read < 1) {
      return;
    }
  }
}

}  // namespace telnet
}  // namespace esphome
