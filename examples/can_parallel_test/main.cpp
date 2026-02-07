#include <jstm/hal/gpio.hpp>
#include <jstm/hal/hal.hpp>
#include <jstm/log.hpp>
#include <jstm/rtcan/rtcan.hpp>
#include <jstm/rtos/rtos.hpp>

static jstm::rtcan::service* g_rtcan = nullptr;

extern "C" {
void CAN2_TX_IRQHandler() { HAL_CAN_IRQHandler(g_rtcan->can_handle()); }
void CAN2_RX0_IRQHandler() { HAL_CAN_IRQHandler(g_rtcan->can_handle()); }
void CAN2_RX1_IRQHandler() { HAL_CAN_IRQHandler(g_rtcan->can_handle()); }
void CAN2_SCE_IRQHandler() { HAL_CAN_IRQHandler(g_rtcan->can_handle()); }

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

struct tx_params {
  u32 id;
  u32 count;
  u32 delay_ms;
  u32 sent = 0;
  u32 failed = 0;
  bool done = false;
};

static void tx_task(void* arg) {
  auto* p = static_cast<tx_params*>(arg);

  for (u32 i = 0; i < p->count; ++i) {
    rtcan::msg m{};
    m.id = p->id;
    m.dlc = 8;
    m.data[0] = static_cast<u8>(p->id);
    m.data[1] = static_cast<u8>(p->id >> 8);
    m.data[2] = static_cast<u8>(i);
    m.data[3] = static_cast<u8>(i >> 8);
    m.data[4] = static_cast<u8>(i >> 16);
    m.data[5] = static_cast<u8>(i >> 24);
    m.data[6] = static_cast<u8>(p->count);
    m.data[7] = static_cast<u8>(p->count >> 8);

    auto r = g_rtcan->transmit(m);
    if (r) {
      ++p->sent;
    } else {
      ++p->failed;
    }

    if (p->delay_ms > 0) {
      rtos::this_task::delay_ms(p->delay_ms);
    } else {
      taskYIELD();
    }
  }

  p->done = true;
  log::info("task 0x%03lX done: sent=%lu fail=%lu", p->id, p->sent, p->failed);
  vTaskDelete(nullptr);
}

static constexpr u32 NUM_TYPES = 5;
static constexpr u32 MSGS_PER_TYPE = 200;

static tx_params params[NUM_TYPES] = {
    {.id = 0x100, .count = MSGS_PER_TYPE, .delay_ms = 2},
    {.id = 0x200, .count = MSGS_PER_TYPE, .delay_ms = 3},
    {.id = 0x300, .count = MSGS_PER_TYPE, .delay_ms = 2},
    {.id = 0x400, .count = MSGS_PER_TYPE, .delay_ms = 4},
    {.id = 0x500, .count = MSGS_PER_TYPE, .delay_ms = 1},
};

static void monitor_task(void*) {
  while (true) {
    bool all_done = true;
    u32 total_sent = 0;
    u32 total_fail = 0;

    for (u32 i = 0; i < NUM_TYPES; ++i) {
      if (!params[i].done) all_done = false;
      total_sent += params[i].sent;
      total_fail += params[i].failed;
    }

    log::info("progress: sent=%lu fail=%lu / %lu",
              total_sent, total_fail, NUM_TYPES * MSGS_PER_TYPE);

    if (all_done) {
      log::info("========================================");
      log::info("ALL DONE: %lu types x %lu msgs = %lu total",
                NUM_TYPES, MSGS_PER_TYPE, NUM_TYPES * MSGS_PER_TYPE);
      log::info("  sent=%lu  fail=%lu", total_sent, total_fail);
      for (u32 i = 0; i < NUM_TYPES; ++i) {
        log::info("  0x%03lX: sent=%lu fail=%lu",
                  params[i].id, params[i].sent, params[i].failed);
      }
      log::info("========================================");

      while (true) rtos::this_task::delay_ms(10000);
    }

    rtos::this_task::delay_ms(500);
  }
}

int main() {
  hal::system_init();
  log::info("=== CAN2 PARALLEL TX: %lu types x %lu msgs ===",
            NUM_TYPES, MSGS_PER_TYPE);

  rtcan::config cfg{};
  cfg.instance = CAN2;
  cfg.rate = rtcan::bitrate::k500;
  cfg.tx_port = GPIOB;
  cfg.tx_pin = GPIO_PIN_6;
  cfg.rx_port = GPIOB;
  cfg.rx_pin = GPIO_PIN_5;
  cfg.af = GPIO_AF9_CAN2;
  cfg.tx_queue_depth = 32;

  static rtcan::service svc{cfg};
  g_rtcan = &svc;

  auto res = svc.start();
  if (!res) {
    log::error("start failed: %s", res.error().message);
    while (true) {}
  }

  log::info("launching %lu TX tasks...", NUM_TYPES);

  static rtos::task t_mon{"mon", monitor_task, nullptr, 512, 2};
  static rtos::task t0{"tx0", tx_task, &params[0], 512, 3};
  static rtos::task t1{"tx1", tx_task, &params[1], 512, 3};
  static rtos::task t2{"tx2", tx_task, &params[2], 512, 3};
  static rtos::task t3{"tx3", tx_task, &params[3], 512, 3};
  static rtos::task t4{"tx4", tx_task, &params[4], 512, 3};

  rtos::start_scheduler();
  while (true) {}
}
