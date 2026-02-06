#include <jstm/types.hpp>

#include "stm32f7xx_hal.h"

namespace jstm::hal {

void log_uart_init();

static void system_clock_config() {
  RCC_OscInitTypeDef rcc_osc{};
  rcc_osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  rcc_osc.HSEState = RCC_HSE_BYPASS;
  rcc_osc.PLL.PLLState = RCC_PLL_ON;
  rcc_osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  rcc_osc.PLL.PLLM = 8;
  rcc_osc.PLL.PLLN = 432;
  rcc_osc.PLL.PLLP = RCC_PLLP_DIV2;
  rcc_osc.PLL.PLLQ = 9;
  HAL_RCC_OscConfig(&rcc_osc);

  HAL_PWREx_EnableOverDrive();

  RCC_ClkInitTypeDef rcc_clk{};
  rcc_clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                      RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  rcc_clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  rcc_clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
  rcc_clk.APB1CLKDivider = RCC_HCLK_DIV4;
  rcc_clk.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&rcc_clk, FLASH_LATENCY_7);
}

static void enable_dwt_cycle_counter() {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void system_init() {
  HAL_Init();
  system_clock_config();
  enable_dwt_cycle_counter();

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  log_uart_init();
}

}  // namespace jstm::hal
