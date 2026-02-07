
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

struct app_context {
  jstm::rtcan::service* rtcan;
  jstm::rtos::queue<const jstm::rtcan::msg*>* rx_queue;
};

static void producer_task(void* arg) {
  auto* ctx = static_cast<app_context*>(arg);
  using namespace jstm;

  u32 counter = 0;

  while (true) {
    rtcan::msg m{};
    m.id = 0x100;
    m.dlc = 4;
    m.data[0] = static_cast<u8>(counter >> 0);
    m.data[1] = static_cast<u8>(counter >> 8);
    m.data[2] = static_cast<u8>(counter >> 16);
    m.data[3] = static_cast<u8>(counter >> 24);

    auto res = ctx->rtcan->transmit(m);
    if (res) {
      log::info("tx: id=0x%03lX counter=%lu", m.id, counter);
    } else {
      log::warn("tx: failed - %s", res.error().message);
    }

    ++counter;
    rtos::this_task::delay_ms(500);
  }
}

static void subscriber_task(void* arg) {
  auto* ctx = static_cast<app_context*>(arg);
  using namespace jstm;

  while (true) {
    const rtcan::msg* m = nullptr;
    if (ctx->rx_queue->receive(m)) {
      log::info(
          "rx: id=0x%03lX dlc=%u data=[%02X %02X %02X %02X %02X %02X %02X "
          "%02X]",
          m->id, m->dlc, m->data[0], m->data[1], m->data[2], m->data[3],
          m->data[4], m->data[5], m->data[6], m->data[7]);

      ctx->rtcan->msg_consumed(m);
    }
  }
}

int main() {
  using namespace jstm;

  hal::system_init();
  log::info("=== rtcan loopback example ===");

  hal::output_pin led{GPIOB, GPIO_PIN_0};

  rtcan::config cfg{};
  cfg.instance = CAN1;
  cfg.rate = rtcan::bitrate::k500;
  cfg.tx_port = GPIOA;
  cfg.tx_pin = GPIO_PIN_12;
  cfg.rx_port = GPIOA;
  cfg.rx_pin = GPIO_PIN_11;
  cfg.af = GPIO_AF9_CAN1;
  cfg.loopback = true;
  cfg.tx_queue_depth = 16;
  cfg.rx_pool_size = 32;

  static rtcan::service rtcan_svc{cfg};
  g_rtcan = &rtcan_svc;

  static rtos::queue<const rtcan::msg*> rx_queue{16};
  rtcan_svc.subscribe(0x100, rx_queue);

  auto res = rtcan_svc.start();
  if (!res) {
    log::error("rtcan start failed: %s", res.error().message);
    while (true) {
    }
  }

  static app_context ctx{&rtcan_svc, &rx_queue};

  static rtos::task producer{"producer", producer_task, &ctx, 512, 2};
  static rtos::task subscriber{"subscriber", subscriber_task, &ctx, 512, 2};

  static rtos::task heartbeat{"heartbeat",
                              [](void* arg) {
                                auto* l = static_cast<hal::output_pin*>(arg);
                                while (true) {
                                  l->toggle();
                                  rtos::this_task::delay_ms(1000);
                                }
                              },
                              &led, 256, 1};

  rtos::start_scheduler();

  while (true) {
  }
}
