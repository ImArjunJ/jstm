#pragma once

#include <jstm/result.hpp>
#include <jstm/types.hpp>
#include <span>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

namespace jstm::rtos {

inline void start_scheduler() { vTaskStartScheduler(); }

inline u32 tick_count() { return xTaskGetTickCount(); }

inline void delay(u32 ticks) { vTaskDelay(ticks); }

inline void delay_ms(u32 ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

class task {
 public:
  using function_t = void (*)(void*);

  task(const char* name, function_t fn, void* param = nullptr,
       u16 stack_depth = 256, u32 priority = 1) {
    xTaskCreate(fn, name, stack_depth, param, priority, &handle_);
  }

  ~task() {
    if (handle_) vTaskDelete(handle_);
  }

  task(const task&) = delete;
  task& operator=(const task&) = delete;

  task(task&& other) noexcept : handle_{other.handle_} {
    other.handle_ = nullptr;
  }

  task& operator=(task&& other) noexcept {
    if (this != &other) {
      if (handle_) vTaskDelete(handle_);
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }

  void suspend() {
    if (handle_) vTaskSuspend(handle_);
  }
  void resume() {
    if (handle_) vTaskResume(handle_);
  }

  u32 priority() const { return uxTaskPriorityGet(handle_); }
  void set_priority(u32 prio) { vTaskPrioritySet(handle_, prio); }

  u32 stack_high_water_mark() const {
    return uxTaskGetStackHighWaterMark(handle_);
  }

  bool valid() const { return handle_ != nullptr; }
  TaskHandle_t handle() const { return handle_; }

 private:
  TaskHandle_t handle_ = nullptr;
};

class mutex {
 public:
  mutex() : handle_{xSemaphoreCreateMutex()} {}

  ~mutex() {
    if (handle_) vSemaphoreDelete(handle_);
  }

  mutex(const mutex&) = delete;
  mutex& operator=(const mutex&) = delete;

  mutex(mutex&& other) noexcept : handle_{other.handle_} {
    other.handle_ = nullptr;
  }

  mutex& operator=(mutex&& other) noexcept {
    if (this != &other) {
      if (handle_) vSemaphoreDelete(handle_);
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }

  bool lock(u32 timeout_ticks = portMAX_DELAY) {
    return xSemaphoreTake(handle_, timeout_ticks) == pdTRUE;
  }

  void unlock() { xSemaphoreGive(handle_); }

  bool valid() const { return handle_ != nullptr; }
  SemaphoreHandle_t handle() const { return handle_; }

 private:
  SemaphoreHandle_t handle_;
};

class lock_guard {
 public:
  explicit lock_guard(mutex& m) : mtx_{m} { mtx_.lock(); }
  ~lock_guard() { mtx_.unlock(); }

  lock_guard(const lock_guard&) = delete;
  lock_guard& operator=(const lock_guard&) = delete;

 private:
  mutex& mtx_;
};

class binary_semaphore {
 public:
  binary_semaphore() : handle_{xSemaphoreCreateBinary()} {}

  ~binary_semaphore() {
    if (handle_) vSemaphoreDelete(handle_);
  }

  binary_semaphore(const binary_semaphore&) = delete;
  binary_semaphore& operator=(const binary_semaphore&) = delete;

  binary_semaphore(binary_semaphore&& other) noexcept : handle_{other.handle_} {
    other.handle_ = nullptr;
  }

  binary_semaphore& operator=(binary_semaphore&& other) noexcept {
    if (this != &other) {
      if (handle_) vSemaphoreDelete(handle_);
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }

  bool take(u32 timeout_ticks = portMAX_DELAY) {
    return xSemaphoreTake(handle_, timeout_ticks) == pdTRUE;
  }

  void give() { xSemaphoreGive(handle_); }

  void give_from_isr() {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(handle_, &woken);
    portYIELD_FROM_ISR(woken);
  }

  bool valid() const { return handle_ != nullptr; }

 private:
  SemaphoreHandle_t handle_;
};

class counting_semaphore {
 public:
  counting_semaphore(u32 max_count, u32 initial_count = 0)
      : handle_{xSemaphoreCreateCounting(max_count, initial_count)} {}

  ~counting_semaphore() {
    if (handle_) vSemaphoreDelete(handle_);
  }

  counting_semaphore(const counting_semaphore&) = delete;
  counting_semaphore& operator=(const counting_semaphore&) = delete;

  counting_semaphore(counting_semaphore&& other) noexcept
      : handle_{other.handle_} {
    other.handle_ = nullptr;
  }

  counting_semaphore& operator=(counting_semaphore&& other) noexcept {
    if (this != &other) {
      if (handle_) vSemaphoreDelete(handle_);
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }

  bool take(u32 timeout_ticks = portMAX_DELAY) {
    return xSemaphoreTake(handle_, timeout_ticks) == pdTRUE;
  }

  void give() { xSemaphoreGive(handle_); }

  void give_from_isr() {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(handle_, &woken);
    portYIELD_FROM_ISR(woken);
  }

  u32 count() const { return uxSemaphoreGetCount(handle_); }

  bool valid() const { return handle_ != nullptr; }

 private:
  SemaphoreHandle_t handle_;
};

template <typename T>
class queue {
 public:
  explicit queue(u32 length) : handle_{xQueueCreate(length, sizeof(T))} {}

  ~queue() {
    if (handle_) vQueueDelete(handle_);
  }

  queue(const queue&) = delete;
  queue& operator=(const queue&) = delete;

  queue(queue&& other) noexcept : handle_{other.handle_} {
    other.handle_ = nullptr;
  }

  queue& operator=(queue&& other) noexcept {
    if (this != &other) {
      if (handle_) vQueueDelete(handle_);
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }

  bool send(const T& item, u32 timeout_ticks = portMAX_DELAY) {
    return xQueueSend(handle_, &item, timeout_ticks) == pdTRUE;
  }

  bool send_to_front(const T& item, u32 timeout_ticks = portMAX_DELAY) {
    return xQueueSendToFront(handle_, &item, timeout_ticks) == pdTRUE;
  }

  bool send_from_isr(const T& item) {
    BaseType_t woken = pdFALSE;
    auto ok = xQueueSendFromISR(handle_, &item, &woken);
    portYIELD_FROM_ISR(woken);
    return ok == pdTRUE;
  }

  bool receive(T& item, u32 timeout_ticks = portMAX_DELAY) {
    return xQueueReceive(handle_, &item, timeout_ticks) == pdTRUE;
  }

  bool receive_from_isr(T& item) {
    BaseType_t woken = pdFALSE;
    auto ok = xQueueReceiveFromISR(handle_, &item, &woken);
    portYIELD_FROM_ISR(woken);
    return ok == pdTRUE;
  }

  bool peek(T& item, u32 timeout_ticks = 0) const {
    return xQueuePeek(handle_, &item, timeout_ticks) == pdTRUE;
  }

  u32 count() const { return uxQueueMessagesWaiting(handle_); }

  u32 spaces() const { return uxQueueSpacesAvailable(handle_); }

  void reset() { xQueueReset(handle_); }

  bool valid() const { return handle_ != nullptr; }

 private:
  QueueHandle_t handle_;
};

class software_timer {
 public:
  using callback_t = void (*)(TimerHandle_t);

  software_timer(const char* name, u32 period_ticks, bool auto_reload,
                 callback_t callback)
      : handle_{xTimerCreate(name, period_ticks, auto_reload ? pdTRUE : pdFALSE,
                             nullptr, callback)} {}

  ~software_timer() {
    if (handle_) xTimerDelete(handle_, portMAX_DELAY);
  }

  software_timer(const software_timer&) = delete;
  software_timer& operator=(const software_timer&) = delete;

  software_timer(software_timer&& other) noexcept : handle_{other.handle_} {
    other.handle_ = nullptr;
  }

  software_timer& operator=(software_timer&& other) noexcept {
    if (this != &other) {
      if (handle_) xTimerDelete(handle_, portMAX_DELAY);
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }

  bool start(u32 timeout_ticks = portMAX_DELAY) {
    return xTimerStart(handle_, timeout_ticks) == pdTRUE;
  }

  bool stop(u32 timeout_ticks = portMAX_DELAY) {
    return xTimerStop(handle_, timeout_ticks) == pdTRUE;
  }

  bool reset(u32 timeout_ticks = portMAX_DELAY) {
    return xTimerReset(handle_, timeout_ticks) == pdTRUE;
  }

  bool set_period(u32 new_period_ticks, u32 timeout_ticks = portMAX_DELAY) {
    return xTimerChangePeriod(handle_, new_period_ticks, timeout_ticks) ==
           pdTRUE;
  }

  bool is_active() const { return xTimerIsTimerActive(handle_) != pdFALSE; }

  bool valid() const { return handle_ != nullptr; }

 private:
  TimerHandle_t handle_;
};

namespace this_task {

inline void yield() { taskYIELD(); }

inline void delay(u32 ticks) { vTaskDelay(ticks); }

inline void delay_ms(u32 ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

inline void delay_until(u32& previous_wake, u32 increment) {
  vTaskDelayUntil(reinterpret_cast<TickType_t*>(&previous_wake), increment);
}

inline TaskHandle_t handle() { return xTaskGetCurrentTaskHandle(); }

inline void suspend() { vTaskSuspend(nullptr); }

}  // namespace this_task

}  // namespace jstm::rtos
