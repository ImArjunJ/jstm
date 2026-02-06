#pragma once

#include <jstm/types.hpp>

#include "stm32f7xx_hal.h"

namespace jstm::hal {

void system_init();

class output_pin {
 public:
  output_pin(GPIO_TypeDef* port, u16 pin, bool initial = false)
      : port_{port}, pin_{pin} {
    GPIO_InitTypeDef gpio{};
    gpio.Pin = pin_;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port_, &gpio);
    HAL_GPIO_WritePin(port_, pin_, initial ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }

  void high() { HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_SET); }
  void low() { HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_RESET); }

  void set(bool v) {
    HAL_GPIO_WritePin(port_, pin_, v ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }

  void toggle() { HAL_GPIO_TogglePin(port_, pin_); }

  GPIO_TypeDef* port() const { return port_; }
  u16 pin() const { return pin_; }

  output_pin(const output_pin&) = delete;
  output_pin& operator=(const output_pin&) = delete;

 private:
  GPIO_TypeDef* port_;
  u16 pin_;
};

enum class pull : u8 { none, up, down };

class input_pin {
 public:
  input_pin(GPIO_TypeDef* port, u16 pin, pull p = pull::none)
      : port_{port}, pin_{pin} {
    GPIO_InitTypeDef gpio{};
    gpio.Pin = pin_;
    gpio.Mode = GPIO_MODE_INPUT;
    switch (p) {
      case pull::up:
        gpio.Pull = GPIO_PULLUP;
        break;
      case pull::down:
        gpio.Pull = GPIO_PULLDOWN;
        break;
      case pull::none:
        gpio.Pull = GPIO_NOPULL;
        break;
    }
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port_, &gpio);
  }

  bool read() const { return HAL_GPIO_ReadPin(port_, pin_) == GPIO_PIN_SET; }

  GPIO_TypeDef* port() const { return port_; }
  u16 pin() const { return pin_; }

  input_pin(const input_pin&) = delete;
  input_pin& operator=(const input_pin&) = delete;

 private:
  GPIO_TypeDef* port_;
  u16 pin_;
};

class cs_guard {
 public:
  explicit cs_guard(output_pin& cs) : cs_{cs} { cs_.low(); }
  ~cs_guard() { cs_.high(); }

  cs_guard(const cs_guard&) = delete;
  cs_guard& operator=(const cs_guard&) = delete;

 private:
  output_pin& cs_;
};

}  // namespace jstm::hal
