
#include <jstm/hal/gpio.hpp>
#include <jstm/hal/hal.hpp>
#include <jstm/log.hpp>
#include <jstm/types.hpp>

#include "stm32f7xx_hal.h"

using namespace jstm;

static CAN_HandleTypeDef hcan{};

extern "C" void CAN2_TX_IRQHandler() { HAL_CAN_IRQHandler(&hcan); }
extern "C" void CAN2_RX0_IRQHandler() { HAL_CAN_IRQHandler(&hcan); }
extern "C" void CAN2_SCE_IRQHandler() { HAL_CAN_IRQHandler(&hcan); }

static volatile bool tx_complete = false;
static volatile bool rx_pending = false;
static volatile u32 rx_id = 0;
static volatile u8 rx_dlc = 0;
static volatile u8 rx_data[8] = {};

extern "C" void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef*) {
  tx_complete = true;
}
extern "C" void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef*) {
  tx_complete = true;
}
extern "C" void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef*) {
  tx_complete = true;
}
extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* h) {
  CAN_RxHeaderTypeDef hdr{};
  u8 data[8]{};
  if (HAL_CAN_GetRxMessage(h, CAN_RX_FIFO0, &hdr, data) == HAL_OK) {
    rx_id = hdr.StdId;
    rx_dlc = hdr.DLC;
    for (u32 i = 0; i < 8; ++i) rx_data[i] = data[i];
    rx_pending = true;
  }
}
extern "C" void HAL_CAN_ErrorCallback(CAN_HandleTypeDef* h) {
  u32 esr = h->Instance->ESR;
  log::warn("ERR_CB esr=0x%08lX err=0x%08lX TEC=%lu REC=%lu", esr, h->ErrorCode,
            (esr >> 16) & 0xFF, (esr >> 24) & 0xFF);
}

static void dump_regs(const char* label) {
  auto* r = CAN2;
  log::info("%s MCR=0x%08lX MSR=0x%08lX ESR=0x%08lX BTR=0x%08lX", label, r->MCR,
            r->MSR, r->ESR, r->BTR);
}

int main() {
  hal::system_init();
  log::info("=== BARE METAL CAN2 TEST: PB6(TX) PB5(RX) ===");

  __HAL_RCC_CAN1_CLK_ENABLE();
  __HAL_RCC_CAN2_CLK_ENABLE();

  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF9_CAN2;
  gpio.Pin = GPIO_PIN_6;
  HAL_GPIO_Init(GPIOB, &gpio);
  gpio.Pin = GPIO_PIN_5;
  HAL_GPIO_Init(GPIOB, &gpio);

  hcan.Instance = CAN2;
  hcan.Init.Prescaler = 6;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_4TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = ENABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = ENABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;

  auto r = HAL_CAN_Init(&hcan);
  log::info("HAL_CAN_Init = %d", static_cast<int>(r));
  dump_regs("post-init");

  CAN_FilterTypeDef filt{};
  filt.FilterBank = 14;
  filt.FilterMode = CAN_FILTERMODE_IDMASK;
  filt.FilterScale = CAN_FILTERSCALE_32BIT;
  filt.FilterIdHigh = 0;
  filt.FilterIdLow = 0;
  filt.FilterMaskIdHigh = 0;
  filt.FilterMaskIdLow = 0;
  filt.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  filt.FilterActivation = ENABLE;
  filt.SlaveStartFilterBank = 14;
  HAL_CAN_ConfigFilter(&hcan, &filt);

  HAL_NVIC_SetPriority(CAN2_TX_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(CAN2_TX_IRQn);
  HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
  HAL_NVIC_SetPriority(CAN2_SCE_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(CAN2_SCE_IRQn);

  HAL_CAN_ActivateNotification(
      &hcan, CAN_IT_TX_MAILBOX_EMPTY | CAN_IT_RX_FIFO0_MSG_PENDING |
                 CAN_IT_ERROR_WARNING | CAN_IT_ERROR_PASSIVE | CAN_IT_BUSOFF |
                 CAN_IT_LAST_ERROR_CODE);

  r = HAL_CAN_Start(&hcan);
  log::info("HAL_CAN_Start = %d", static_cast<int>(r));
  dump_regs("running");

  log::info("PB5(RX) IDR=%lu", (GPIOB->IDR >> 5) & 1);
  log::info("waiting 3s for candapter RX before TX...");

  for (int i = 0; i < 30; ++i) {
    if (rx_pending) {
      log::info("RX! id=0x%03lX dlc=%u data=[%02X %02X %02X %02X]", rx_id,
                rx_dlc, rx_data[0], rx_data[1], rx_data[2], rx_data[3]);
      rx_pending = false;
    }
    HAL_Delay(100);
  }

  log::info("--- starting TX: 0x100 every 1s ---");
  u32 seq = 0;
  while (true) {
    CAN_TxHeaderTypeDef txh{};
    txh.StdId = 0x100;
    txh.RTR = CAN_RTR_DATA;
    txh.IDE = CAN_ID_STD;
    txh.DLC = 4;
    u8 data[4];
    data[0] = static_cast<u8>(seq);
    data[1] = static_cast<u8>(seq >> 8);
    data[2] = static_cast<u8>(seq >> 16);
    data[3] = static_cast<u8>(seq >> 24);

    u32 mailbox = 0;
    tx_complete = false;
    auto s = HAL_CAN_AddTxMessage(&hcan, &txh, data, &mailbox);
    if (s != HAL_OK) {
      log::warn("AddTxMsg fail=%d free=%lu", static_cast<int>(s),
                HAL_CAN_GetTxMailboxesFreeLevel(&hcan));
    } else {
      log::info("tx seq=%lu mb=%lu", seq, mailbox);
      u32 wait = 0;
      while (!tx_complete && wait < 100) {
        HAL_Delay(1);
        ++wait;
      }
      if (tx_complete) {
        log::info("  tx_complete ok (%lu ms)", wait);
      } else {
        u32 esr = CAN2->ESR;
        log::warn("  tx TIMEOUT esr=0x%08lX TEC=%lu REC=%lu", esr,
                  (esr >> 16) & 0xFF, (esr >> 24) & 0xFF);
      }
    }

    if (rx_pending) {
      log::info("RX! id=0x%03lX dlc=%u data=[%02X %02X %02X %02X]", rx_id,
                rx_dlc, rx_data[0], rx_data[1], rx_data[2], rx_data[3]);
      rx_pending = false;
    }

    dump_regs("loop");
    ++seq;
    HAL_Delay(1000);
  }
}
