
#include <jstm/hal/gpio.hpp>
#include <jstm/hal/hal.hpp>
#include <jstm/log.hpp>
#include <jstm/rtcan/rtcan.hpp>
#include <jstm/rtos/rtos.hpp>

static jstm::rtcan::service* g_rtcan = nullptr;

extern "C" {

void CAN1_TX_IRQHandler() { HAL_CAN_IRQHandler(g_rtcan->can_handle()); }
void CAN1_RX0_IRQHandler() { HAL_CAN_IRQHandler(g_rtcan->can_handle()); }
void CAN1_RX1_IRQHandler() { HAL_CAN_IRQHandler(g_rtcan->can_handle()); }
void CAN1_SCE_IRQHandler() { HAL_CAN_IRQHandler(g_rtcan->can_handle()); }

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef*) {
  if (g_rtcan) g_rtcan->handle_tx_complete_isr();
}
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef*) {
  if (g_rtcan) g_rtcan->handle_tx_complete_isr();
}
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef*) {
  if (g_rtcan) g_rtcan->handle_tx_complete_isr();
}
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef*) {
  if (g_rtcan) g_rtcan->handle_rx_isr(CAN_RX_FIFO0);
}
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef*) {
  if (g_rtcan) g_rtcan->handle_rx_isr(CAN_RX_FIFO1);
}
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef*) {
  if (g_rtcan) g_rtcan->handle_error_isr();
}
}

using namespace jstm;

struct stats {
  u32 tx_ok = 0;
  u32 tx_fail = 0;
  u32 rx_a = 0;
  u32 rx_b = 0;
  u32 rx_multi = 0;
  u32 seq_err = 0;
  u32 data_err = 0;
};

static stats g_stats;

static void multi_id_producer(void* arg) {
  auto* svc = static_cast<rtcan::service*>(arg);
  u32 seq = 0;

  while (true) {
    for (u32 id : {0x100u, 0x200u, 0x300u}) {
      rtcan::msg m{};
      m.id = id;
      m.dlc = 8;
      m.data[0] = static_cast<u8>(seq);
      m.data[1] = static_cast<u8>(seq >> 8);
      m.data[2] = static_cast<u8>(seq >> 16);
      m.data[3] = static_cast<u8>(seq >> 24);
      m.data[4] = static_cast<u8>(id);
      m.data[5] = static_cast<u8>(id >> 8);
      m.data[6] = 0xCA;
      m.data[7] = 0xFE;

      if (svc->transmit(m))
        ++g_stats.tx_ok;
      else
        ++g_stats.tx_fail;
    }
    ++seq;
    rtos::this_task::delay_ms(50);
  }
}

static void subscriber_a(void* arg) {
  auto* q = static_cast<rtos::queue<const rtcan::msg*>*>(arg);
  u32 last_seq_100 = 0xFFFFFFFF;

  while (true) {
    const rtcan::msg* m = nullptr;
    if (q->receive(m, pdMS_TO_TICKS(200))) {
      u32 seq = m->data[0] | (m->data[1] << 8) | (m->data[2] << 16) |
                (m->data[3] << 24);

      if (m->data[6] != 0xCA || m->data[7] != 0xFE) ++g_stats.data_err;

      if (m->id == 0x100) {
        if (last_seq_100 != 0xFFFFFFFF && seq != last_seq_100 + 1)
          ++g_stats.seq_err;
        last_seq_100 = seq;
      }

      ++g_stats.rx_a;
      g_rtcan->msg_consumed(m);
    }
  }
}

static void subscriber_b(void* arg) {
  auto* q = static_cast<rtos::queue<const rtcan::msg*>*>(arg);

  while (true) {
    const rtcan::msg* m = nullptr;
    if (q->receive(m, pdMS_TO_TICKS(200))) {
      if (m->data[6] != 0xCA || m->data[7] != 0xFE) ++g_stats.data_err;
      ++g_stats.rx_b;
      g_rtcan->msg_consumed(m);
    }
  }
}

static void multi_sub_listener(void* arg) {
  auto* q = static_cast<rtos::queue<const rtcan::msg*>*>(arg);

  while (true) {
    const rtcan::msg* m = nullptr;
    if (q->receive(m, pdMS_TO_TICKS(200))) {
      if (m->data[6] != 0xCA || m->data[7] != 0xFE) ++g_stats.data_err;
      ++g_stats.rx_multi;
      g_rtcan->msg_consumed(m);
    }
  }
}

static void burst_producer(void* arg) {
  auto* svc = static_cast<rtcan::service*>(arg);

  while (true) {
    rtos::this_task::delay_ms(3000);

    u32 ok = 0, fail = 0;
    for (u32 i = 0; i < 32; ++i) {
      rtcan::msg m{};
      m.id = 0x400;
      m.dlc = 2;
      m.data[0] = static_cast<u8>(i);
      m.data[1] = 0xBB;
      if (svc->transmit(m))
        ++ok;
      else
        ++fail;
    }
    log::info("burst: 32 msgs sent, ok=%lu fail=%lu", ok, fail);
    g_stats.tx_ok += ok;
    g_stats.tx_fail += fail;
  }
}

static void lifecycle_test(void* arg) {
  auto* svc = static_cast<rtcan::service*>(arg);

  static rtos::queue<const rtcan::msg*> temp_q{8};

  while (true) {
    rtos::this_task::delay_ms(5000);

    auto r = svc->subscribe(0x100, temp_q);
    if (!r) {
      log::warn("lifecycle: subscribe failed: %s", r.error().message);
      continue;
    }
    u16 cnt = svc->subscriber_count(0x100);
    log::info("lifecycle: subscribed to 0x100, count=%u", cnt);

    rtos::this_task::delay_ms(2000);

    u32 drained = 0;
    const rtcan::msg* m = nullptr;
    while (temp_q.receive(m, 0)) {
      svc->msg_consumed(m);
      ++drained;
    }

    auto u = svc->unsubscribe(0x100, temp_q);
    cnt = svc->subscriber_count(0x100);
    log::info("lifecycle: unsubscribed, drained=%lu, count=%u, ok=%d", drained,
              cnt, u.has_value());
  }
}

int main() {
  hal::system_init();
  log::info("=== rtcan stress test ===");

  rtcan::config cfg{};
  cfg.instance = CAN1;
  cfg.rate = rtcan::bitrate::k500;
  cfg.tx_port = GPIOA;
  cfg.tx_pin = GPIO_PIN_12;
  cfg.rx_port = GPIOA;
  cfg.rx_pin = GPIO_PIN_11;
  cfg.af = GPIO_AF9_CAN1;
  cfg.loopback = true;
  cfg.tx_queue_depth = 32;
  cfg.rx_pool_size = 64;
  cfg.hashmap_size = 32;
  cfg.max_subscribers = 32;

  static rtcan::service svc{cfg};
  g_rtcan = &svc;

  static rtos::queue<const rtcan::msg*> q_a{16};
  static rtos::queue<const rtcan::msg*> q_b{16};
  static rtos::queue<const rtcan::msg*> q_multi{16};

  svc.subscribe(0x100, q_a);
  svc.subscribe(0x200, q_a);
  svc.subscribe(0x300, q_a);
  svc.subscribe(0x200, q_b);
  svc.subscribe(0x100, q_multi);

  log::info("subscribers: 0x100=%u 0x200=%u 0x300=%u",
            svc.subscriber_count(0x100), svc.subscriber_count(0x200),
            svc.subscriber_count(0x300));

  auto res = svc.start();
  if (!res) {
    log::error("start failed: %s", res.error().message);
    while (true) {
    }
  }

  static rtos::task t_prod{"producer", multi_id_producer, &svc, 512, 3};
  static rtos::task t_sub_a{"sub_a", subscriber_a, &q_a, 512, 2};
  static rtos::task t_sub_b{"sub_b", subscriber_b, &q_b, 512, 2};
  static rtos::task t_multi{"multi", multi_sub_listener, &q_multi, 512, 2};
  static rtos::task t_burst{"burst", burst_producer, &svc, 512, 3};
  static rtos::task t_life{"lifecycle", lifecycle_test, &svc, 512, 2};

  static hal::output_pin led{GPIOB, GPIO_PIN_0};

  static rtos::task heartbeat{"heartbeat",
                              [](void* arg) {
                                auto* l = static_cast<hal::output_pin*>(arg);
                                while (true) {
                                  l->toggle();
                                  auto e = g_rtcan->error();
                                  log::info(
                                      "--- tx=%lu/%lu rx_a=%lu rx_b=%lu "
                                      "multi=%lu seq_err=%lu data_err=%lu "
                                      "err=0x%04lX ---",
                                      g_stats.tx_ok, g_stats.tx_fail,
                                      g_stats.rx_a, g_stats.rx_b,
                                      g_stats.rx_multi, g_stats.seq_err,
                                      g_stats.data_err, static_cast<u32>(e));
                                  if (e != rtcan::rtcan_error::none)
                                    g_rtcan->clear_error();
                                  rtos::this_task::delay_ms(2000);
                                }
                              },
                              &led, 512, 1};

  rtos::start_scheduler();

  while (true) {
  }
}
