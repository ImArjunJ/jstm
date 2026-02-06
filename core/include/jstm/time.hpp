#pragma once

#include <jstm/types.hpp>

#include "stm32f7xx_hal.h"

namespace jstm {

inline void delay_ms(u32 ms) { HAL_Delay(ms); }

inline void delay_us(u32 us) {
  u32 start = DWT->CYCCNT;
  u32 ticks = (HAL_RCC_GetHCLKFreq() / 1'000'000) * us;
  while ((DWT->CYCCNT - start) < ticks) {
  }
}

inline u32 millis() { return HAL_GetTick(); }

}  // namespace jstm
