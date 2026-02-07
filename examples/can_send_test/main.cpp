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

static void tx_task(void* arg) {
  auto* svc = static_cast<rtcan::service*>(arg);
  u32 seq = 0;

  while (true) {
    rtcan::msg m{};
    m.id = 0x100;
    m.dlc = 4;
    m.data[0] = static_cast<u8>(seq);
    m.data[1] = static_cast<u8>(seq >> 8);
    m.data[2] = static_cast<u8>(seq >> 16);
    m.data[3] = static_cast<u8>(seq >> 24);

    auto r = svc->transmit(m);
    if (r) {
      log::info("tx 0x100 seq=%lu", seq);
    } else {
      log::warn("tx fail: %s", r.error().message);
    }

    ++seq;
    rtos::this_task::delay_ms(500);
  }
}

int main() {
  hal::system_init();
  log::info("=== CAN2 SEND TEST: PB6(TX) PB5(RX) @ 500k ===");

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

  auto res = svc.start();
  if (!res) {
    log::error("start failed: %s", res.error().message);
    while (true) {}
  }

  log::info("sending 0x100 every 500ms");
  static rtos::task t_tx{"tx", tx_task, &svc, 512, 3};

  static hal::output_pin led{GPIOB, GPIO_PIN_0};
  static rtos::task heartbeat{"hb", [](void* arg) {
    auto* l = static_cast<hal::output_pin*>(arg);
    while (true) {
      l->toggle();
      auto e = g_rtcan->error();
      if (e != rtcan::rtcan_error::none) {
        log::warn("can err=0x%04lX", static_cast<u32>(e));
        g_rtcan->clear_error();
      }
      rtos::this_task::delay_ms(1000);
    }
  }, &led, 512, 1};

  rtos::start_scheduler();
  while (true) {}
}
