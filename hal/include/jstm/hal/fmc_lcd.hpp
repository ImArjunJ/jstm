#pragma once

#include <jstm/types.hpp>

#include "stm32f7xx_hal.h"

namespace jstm::hal {

struct fmc_lcd_addresses {
  volatile u16* cmd;
  volatile u16* data;
};

inline fmc_lcd_addresses fmc_lcd_init() {
  __HAL_RCC_FMC_CLK_ENABLE();
  __HAL_RCC_SYSCFG_CLK_ENABLE();

  HAL_EnableFMCMemorySwapping();

  GPIO_InitTypeDef gpio{};
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF12_FMC;

  gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7 |
             GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOD, &gpio);

  gpio.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |
             GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOE, &gpio);

  gpio.Pin = GPIO_PIN_0;
  HAL_GPIO_Init(GPIOF, &gpio);

  static SRAM_HandleTypeDef hsram{};
  hsram.Instance = FMC_NORSRAM_DEVICE;
  hsram.Extended = FMC_NORSRAM_EXTENDED_DEVICE;

  FMC_NORSRAM_TimingTypeDef timing{};
  timing.AddressSetupTime = 3;
  timing.AddressHoldTime = 15;
  timing.DataSetupTime = 2;
  timing.BusTurnAroundDuration = 2;
  timing.CLKDivision = 16;
  timing.DataLatency = 17;
  timing.AccessMode = FMC_ACCESS_MODE_A;

  hsram.Init.NSBank = FMC_NORSRAM_BANK1;
  hsram.Init.DataAddressMux = FMC_DATA_ADDRESS_MUX_DISABLE;
  hsram.Init.MemoryType = FMC_MEMORY_TYPE_SRAM;
  hsram.Init.MemoryDataWidth = FMC_NORSRAM_MEM_BUS_WIDTH_16;
  hsram.Init.BurstAccessMode = FMC_BURST_ACCESS_MODE_DISABLE;
  hsram.Init.WaitSignalPolarity = FMC_WAIT_SIGNAL_POLARITY_LOW;
  hsram.Init.WaitSignalActive = FMC_WAIT_TIMING_BEFORE_WS;
  hsram.Init.WriteOperation = FMC_WRITE_OPERATION_ENABLE;
  hsram.Init.WaitSignal = FMC_WAIT_SIGNAL_DISABLE;
  hsram.Init.ExtendedMode = FMC_EXTENDED_MODE_DISABLE;
  hsram.Init.AsynchronousWait = FMC_ASYNCHRONOUS_WAIT_DISABLE;
  hsram.Init.WriteBurst = FMC_WRITE_BURST_DISABLE;
  hsram.Init.ContinuousClock = FMC_CONTINUOUS_CLOCK_SYNC_ONLY;
  hsram.Init.WriteFifo = FMC_WRITE_FIFO_ENABLE;
  hsram.Init.PageSize = FMC_PAGE_SIZE_NONE;

  HAL_SRAM_Init(&hsram, &timing, nullptr);

  constexpr u32 bank1_base = 0xC0000000UL;
  constexpr u32 a0_offset = 1UL << (0 + 1);

  return {
      reinterpret_cast<volatile u16*>(bank1_base),
      reinterpret_cast<volatile u16*>(bank1_base | a0_offset),
  };
}

}  // namespace jstm::hal
