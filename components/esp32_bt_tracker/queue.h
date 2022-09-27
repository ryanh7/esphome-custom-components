#pragma once
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include <queue>
#include <mutex>

#ifdef USE_ESP32

#include <esp_gap_bt_api.h>
#include <esp_gattc_api.h>

/*
 * BLE events come in from a separate Task (thread) in the ESP32 stack. Rather
 * than trying to deal wth various locking strategies, all incoming GAP and GATT
 * events will simply be placed on a semaphore guarded queue. The next time the
 * component runs loop(), these events are popped off the queue and handed at
 * this safer time.
 */

namespace esphome {
namespace esp32_bt_tracker {

template<class T> class Queue {
 public:
  Queue() { m = xSemaphoreCreateMutex(); }

  void push(T *element) {
    if (element == nullptr)
      return;
    if (xSemaphoreTake(m, 0)) {
      q.push(element);
      xSemaphoreGive(m);
    }
  }

  T *pop() {
    T *element = nullptr;

    if (xSemaphoreTake(m, 5L / portTICK_PERIOD_MS)) {
      if (!q.empty()) {
        element = q.front();
        q.pop();
      }
      xSemaphoreGive(m);
    }
    return element;
  }

 protected:
  std::queue<T *> q;
  SemaphoreHandle_t m;
};



}  // namespace esp32_bt_tracker
}  // namespace esphome

#endif
