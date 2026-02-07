#pragma once

#include <atomic>
#include <jstm/result.hpp>
#include <jstm/rtos/rtos.hpp>
#include <jstm/types.hpp>

#include "stm32f7xx_hal.h"

namespace jstm::rtcan {

enum class bitrate : u32 {
  k125 = 125'000,
  k250 = 250'000,
  k500 = 500'000,
  k1000 = 1'000'000,
};

struct config {
  CAN_TypeDef* instance = CAN1;
  bitrate rate = bitrate::k500;

  GPIO_TypeDef* tx_port = GPIOA;
  u16 tx_pin = GPIO_PIN_12;
  GPIO_TypeDef* rx_port = GPIOA;
  u16 rx_pin = GPIO_PIN_11;
  u8 af = GPIO_AF9_CAN1;

  bool loopback = false;
  bool silent = false;

  u32 thread_priority = 3;
  u16 tx_queue_depth = 16;
  u16 rx_pool_size = 64;
  u16 hashmap_size = 32;
  u16 max_subscribers = 64;
};

struct msg {
  u32 id = 0;
  u8 data[8] = {};
  u8 dlc = 0;
  bool extended = false;
  bool rtr = false;
};

enum class rtcan_error : u32 {
  none = 0x0000'0000,
  init = 0x0000'0001,
  arg = 0x0000'0002,
  memory_full = 0x0000'0004,
  tx_timeout = 0x0000'0008,
  hal = 0x0000'0010,
  internal = 0x8000'0000,
};

inline constexpr rtcan_error operator|(rtcan_error a, rtcan_error b) {
  return static_cast<rtcan_error>(static_cast<u32>(a) | static_cast<u32>(b));
}
inline constexpr rtcan_error operator&(rtcan_error a, rtcan_error b) {
  return static_cast<rtcan_error>(static_cast<u32>(a) & static_cast<u32>(b));
}
inline constexpr rtcan_error& operator|=(rtcan_error& a, rtcan_error b) {
  a = a | b;
  return a;
}

class service {
 public:
  explicit service(const config& cfg);

  ~service();

  service(const service&) = delete;
  service& operator=(const service&) = delete;

  result<void> start();

  result<void> stop();

  result<void> transmit(const msg& m);

  result<void> subscribe(u32 can_id, rtos::queue<const msg*>& q);

  result<void> unsubscribe(u32 can_id, rtos::queue<const msg*>& q);

  u16 subscriber_count(u32 can_id) const;

  void msg_consumed(const msg* m);

  struct filter {
    u32 id = 0;
    u32 mask = 0x7FF;
    bool extended = false;
    u8 fifo = 0;
  };

  result<void> add_filter(const filter& f);

  result<void> add_filter(u32 std_id, u32 mask);

  void clear_filters();

  rtcan_error error() const { return err_; }

  void clear_error() { err_ = rtcan_error::none; }

  CAN_HandleTypeDef* can_handle() { return &hcan_; }

  void handle_tx_complete_isr();
  void handle_rx_isr(u32 fifo);
  void handle_error_isr();

 private:
  struct internal_msg {
    msg payload{};
    std::atomic<u16> refcount{0};
  };

  static constexpr u16 INVALID_INDEX = 0xFFFF;

  struct subscriber_node {
    rtos::queue<const msg*>* q = nullptr;
    u16 next = INVALID_INDEX;
  };

  struct hashmap_slot {
    u32 can_id = 0;
    bool occupied = false;
    u16 first_subscriber = INVALID_INDEX;
    u16 chain_next = INVALID_INDEX;
  };

  void init_peripheral();
  void init_gpio();
  void init_pools();
  void configure_filters();
  void apply_user_filters();
  u16 alloc_subscriber();
  void free_subscriber(u16 idx);
  hashmap_slot* find_or_create_slot(u32 can_id);
  const hashmap_slot* find_slot(u32 can_id) const;
  u32 hash(u32 key) const;

  static void tx_thread_entry(void* arg);
  static void rx_thread_entry(void* arg);

  config cfg_;

  CAN_HandleTypeDef hcan_{};

  rtos::task* tx_task_ = nullptr;
  rtos::task* rx_task_ = nullptr;

  rtos::queue<msg>* tx_queue_ = nullptr;
  rtos::counting_semaphore* tx_mailbox_sem_ = nullptr;

  internal_msg* rx_pool_ = nullptr;
  rtos::queue<u16>* rx_free_list_ = nullptr;
  rtos::queue<u16>* rx_notify_queue_ = nullptr;

  hashmap_slot* map_ = nullptr;
  subscriber_node* subscribers_ = nullptr;
  u16 next_free_subscriber_ = 0;
  u16 subscriber_free_head_ = INVALID_INDEX;

  static constexpr u8 MAX_USER_FILTERS = 14;
  filter user_filters_[MAX_USER_FILTERS]{};
  u8 num_user_filters_ = 0;

  rtcan_error err_ = rtcan_error::none;
  std::atomic<bool> running_{false};
};

}  // namespace jstm::rtcan
