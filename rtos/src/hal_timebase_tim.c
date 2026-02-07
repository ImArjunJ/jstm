#include "stm32f7xx_hal.h"

static TIM_HandleTypeDef htim6;

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
  __HAL_RCC_TIM6_CLK_ENABLE();

  uint32_t pclk1_freq = HAL_RCC_GetPCLK1Freq();
  uint32_t tim_clk = pclk1_freq;

  RCC_ClkInitTypeDef clk_cfg;
  uint32_t latency;
  HAL_RCC_GetClockConfig(&clk_cfg, &latency);
  if (clk_cfg.APB1CLKDivider != RCC_HCLK_DIV1) {
    tim_clk = pclk1_freq * 2;
  }

  uint32_t prescaler = (tim_clk / 10000) - 1;
  uint32_t period = (10000 / 1000) - 1;

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = prescaler;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = period;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
    return HAL_ERROR;
  }

  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, TickPriority, 0);
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);

  return HAL_TIM_Base_Start_IT(&htim6);
}

void HAL_SuspendTick(void) { __HAL_TIM_DISABLE_IT(&htim6, TIM_IT_UPDATE); }

void HAL_ResumeTick(void) { __HAL_TIM_ENABLE_IT(&htim6, TIM_IT_UPDATE); }

void TIM6_DAC_IRQHandler(void) { HAL_TIM_IRQHandler(&htim6); }

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
}
