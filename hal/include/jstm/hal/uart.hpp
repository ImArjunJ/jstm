#pragma once

#include <jstm/result.hpp>
#include <jstm/types.hpp>
#include <span>

#include "stm32f7xx_hal.h"

namespace jstm::hal {

struct uart_config {
  u32 baudrate = 115200;
  GPIO_TypeDef* tx_port = nullptr;
  u16 tx_pin = 0;
  GPIO_TypeDef* rx_port = nullptr;
  u16 rx_pin = 0;
  u8 af = 0;
};

class uart_port {
 public:
  uart_port(USART_TypeDef* instance, uart_config cfg) : config_{cfg} {
    handle_.Instance = instance;
    handle_.Init.BaudRate = config_.baudrate;
    handle_.Init.WordLength = UART_WORDLENGTH_8B;
    handle_.Init.StopBits = UART_STOPBITS_1;
    handle_.Init.Parity = UART_PARITY_NONE;
    handle_.Init.Mode = UART_MODE_TX_RX;
    handle_.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    handle_.Init.OverSampling = UART_OVERSAMPLING_16;

    init_gpio();
    enable_uart_clock(instance);

    HAL_UART_Init(&handle_);
  }

  ~uart_port() { HAL_UART_DeInit(&handle_); }

  uart_port(const uart_port&) = delete;
  uart_port& operator=(const uart_port&) = delete;

  result<usize> write(std::span<const u8> data, u32 timeout_ms = 1000) {
    auto status = HAL_UART_Transmit(&handle_, data.data(),
                                    static_cast<u16>(data.size()), timeout_ms);
    if (status != HAL_OK) {
      return fail(error_code::io_error, "uart transmit failed");
    }
    return ok(data.size());
  }

  result<usize> read(std::span<u8> buf, u32 timeout_ms = 1000) {
    auto status = HAL_UART_Receive(&handle_, buf.data(),
                                   static_cast<u16>(buf.size()), timeout_ms);
    if (status != HAL_OK) {
      return fail(error_code::io_error, "uart receive failed");
    }
    return ok(buf.size());
  }

  UART_HandleTypeDef* handle() { return &handle_; }
  const uart_config& config() const { return config_; }

 private:
  void init_gpio() {
    GPIO_InitTypeDef gpio{};
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = config_.af;

    if (config_.tx_port && config_.tx_pin) {
      enable_gpio_clock(config_.tx_port);
      gpio.Pin = config_.tx_pin;
      HAL_GPIO_Init(config_.tx_port, &gpio);
    }

    if (config_.rx_port && config_.rx_pin) {
      enable_gpio_clock(config_.rx_port);
      gpio.Pin = config_.rx_pin;
      HAL_GPIO_Init(config_.rx_port, &gpio);
    }
  }

  static void enable_gpio_clock(GPIO_TypeDef* port) {
    if (port == GPIOA)
      __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB)
      __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC)
      __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD)
      __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (port == GPIOE)
      __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (port == GPIOF)
      __HAL_RCC_GPIOF_CLK_ENABLE();
    else if (port == GPIOG)
      __HAL_RCC_GPIOG_CLK_ENABLE();
    else if (port == GPIOH)
      __HAL_RCC_GPIOH_CLK_ENABLE();
    else if (port == GPIOI)
      __HAL_RCC_GPIOI_CLK_ENABLE();
  }

  static void enable_uart_clock(USART_TypeDef* inst) {
    if (inst == USART1)
      __HAL_RCC_USART1_CLK_ENABLE();
    else if (inst == USART2)
      __HAL_RCC_USART2_CLK_ENABLE();
    else if (inst == USART3)
      __HAL_RCC_USART3_CLK_ENABLE();
    else if (inst == UART4)
      __HAL_RCC_UART4_CLK_ENABLE();
    else if (inst == UART5)
      __HAL_RCC_UART5_CLK_ENABLE();
    else if (inst == USART6)
      __HAL_RCC_USART6_CLK_ENABLE();
  }

  UART_HandleTypeDef handle_{};
  uart_config config_;
};

}  // namespace jstm::hal
