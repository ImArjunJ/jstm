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

static u32 rx_count = 0;

static void rx_task(void* arg) {
  auto* q = static_cast<rtos::queue<const rtcan::msg*>*>(arg);

  while (true) {
    const rtcan::msg* m = nullptr;
    if (q->receive(m, portMAX_DELAY)) {
      ++rx_count;
      const char* fmt = m->extended ? "EXT" : "STD";
      log::info(
          "[%lu] %s 0x%03lX dlc=%u [%02X %02X %02X %02X %02X %02X %02X %02X]",
          rx_count, fmt, m->id, m->dlc, m->data[0], m->data[1], m->data[2],
          m->data[3], m->data[4], m->data[5], m->data[6], m->data[7]);
      g_rtcan->msg_consumed(m);
    }
  }
}

int main() {
  hal::system_init();
  log::info("=== CAN2 RECV TEST: PB6(TX) PB5(RX) @ 500k ===");

  rtcan::config cfg{};
  cfg.instance = CAN2;
  cfg.rate = rtcan::bitrate::k500;
  cfg.tx_port = GPIOB;
  cfg.tx_pin = GPIO_PIN_6;
  cfg.rx_port = GPIOB;
  cfg.rx_pin = GPIO_PIN_5;
  cfg.af = GPIO_AF9_CAN2;

  static rtcan::service svc{cfg};
  g_rtcan = &svc;

  static rtos::queue<const rtcan::msg*> rx_q{32};
  svc.subscribe_all(rx_q);

  auto res = svc.start();
  if (!res) {
    log::error("start failed: %s", res.error().message);
    while (true) {
    }
  }

  log::info("listening...");

  static rtos::task t_rx{"rx", rx_task, &rx_q, 512, 4};

  static hal::output_pin led{GPIOB, GPIO_PIN_0};
  static rtos::task heartbeat{"hb",
                              [](void*) {
                                while (true) {
                                  led.toggle();
                                  rtos::this_task::delay_ms(500);
                                }
                              },
                              nullptr, 512, 1};

  rtos::start_scheduler();
  while (true) {
  }
}
