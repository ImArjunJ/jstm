#include <jstm/hal/gpio.hpp>
#include <jstm/hal/hal.hpp>
#include <jstm/log.hpp>
#include <jstm/types.hpp>

#include "stm32f7xx_hal.h"

using namespace jstm;

static CAN_HandleTypeDef hcan{};

extern "C" void CAN2_RX0_IRQHandler() { HAL_CAN_IRQHandler(&hcan); }
extern "C" void CAN2_RX1_IRQHandler() { HAL_CAN_IRQHandler(&hcan); }
extern "C" void CAN2_SCE_IRQHandler() { HAL_CAN_IRQHandler(&hcan); }

static volatile u32 rx_count = 0;

extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* h) {
  CAN_RxHeaderTypeDef hdr{};
  u8 data[8]{};
  if (HAL_CAN_GetRxMessage(h, CAN_RX_FIFO0, &hdr, data) != HAL_OK) return;

  u32 id = (hdr.IDE == CAN_ID_STD) ? hdr.StdId : hdr.ExtId;
  const char* fmt = (hdr.IDE == CAN_ID_STD) ? "STD" : "EXT";
  u32 cnt = rx_count + 1;
  rx_count = cnt;

  log::info(
      "[%lu] %s 0x%03lX dlc=%lu [%02X %02X %02X %02X %02X %02X %02X %02X]", cnt,
      fmt, id, static_cast<u32>(hdr.DLC), data[0], data[1], data[2], data[3],
      data[4], data[5], data[6], data[7]);
}

extern "C" void HAL_CAN_ErrorCallback(CAN_HandleTypeDef* h) {
  u32 esr = h->Instance->ESR;
  log::warn("CAN ERR esr=0x%08lX err=0x%08lX TEC=%lu REC=%lu", esr,
            h->ErrorCode, (esr >> 16) & 0xFF, (esr >> 24) & 0xFF);
}

int main() {
  hal::system_init();
  log::info("=== CAN2 RECV TEST: PB6(TX) PB5(RX) @ 500k ===");

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

  if (HAL_CAN_Init(&hcan) != HAL_OK) {
    log::error("HAL_CAN_Init failed");
    while (true) {
    }
  }

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

  HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
  HAL_NVIC_SetPriority(CAN2_RX1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(CAN2_RX1_IRQn);
  HAL_NVIC_SetPriority(CAN2_SCE_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(CAN2_SCE_IRQn);

  HAL_CAN_ActivateNotification(
      &hcan, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING |
                 CAN_IT_ERROR_WARNING | CAN_IT_ERROR_PASSIVE | CAN_IT_BUSOFF |
                 CAN_IT_LAST_ERROR_CODE);

  if (HAL_CAN_Start(&hcan) != HAL_OK) {
    log::error("HAL_CAN_Start failed");
    while (true) {
    }
  }

  log::info("listening...");

  hal::output_pin led{GPIOB, GPIO_PIN_0};
  u32 last_count = 0;

  while (true) {
    led.toggle();

    if (rx_count != last_count) {
      last_count = rx_count;
    }

    HAL_Delay(500);
  }
}
