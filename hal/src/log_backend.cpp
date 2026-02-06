#include <jstm/types.hpp>

#include "stm32f7xx_hal.h"

namespace jstm::hal {

static UART_HandleTypeDef log_uart_handle{};
static bool log_uart_ready = false;

void log_uart_init() {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_UART4_CLK_ENABLE();

  GPIO_InitTypeDef gpio{};
  gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF8_UART4;
  HAL_GPIO_Init(GPIOA, &gpio);

  log_uart_handle.Instance = UART4;
  log_uart_handle.Init.BaudRate = 115200;
  log_uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
  log_uart_handle.Init.StopBits = UART_STOPBITS_1;
  log_uart_handle.Init.Parity = UART_PARITY_NONE;
  log_uart_handle.Init.Mode = UART_MODE_TX;
  log_uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  log_uart_handle.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&log_uart_handle);

  log_uart_ready = true;
}

void log_uart_transmit(const char* data, u32 len) {
  if (!log_uart_ready) return;
  HAL_UART_Transmit(&log_uart_handle, reinterpret_cast<const uint8_t*>(data),
                    static_cast<uint16_t>(len), 100);
}

}  // namespace jstm::hal
