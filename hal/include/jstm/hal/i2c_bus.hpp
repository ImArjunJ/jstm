#pragma once

#include <jstm/result.hpp>
#include <jstm/types.hpp>
#include <span>

#include "stm32f7xx_hal.h"

namespace jstm::hal {

struct i2c_config {
  u32 timing = 0x40912732;
  GPIO_TypeDef* sda_port = nullptr;
  u16 sda_pin = 0;
  GPIO_TypeDef* scl_port = nullptr;
  u16 scl_pin = 0;
  u8 af = 0;
};

class i2c_bus {
 public:
  i2c_bus(I2C_TypeDef* instance, i2c_config cfg) : config_{cfg} {
    handle_.Instance = instance;
    handle_.Init.Timing = config_.timing;
    handle_.Init.OwnAddress1 = 0;
    handle_.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    handle_.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    handle_.Init.OwnAddress2 = 0;
    handle_.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    handle_.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    init_gpio();
    enable_i2c_clock(instance);
    HAL_I2C_Init(&handle_);
  }

  ~i2c_bus() { HAL_I2C_DeInit(&handle_); }

  i2c_bus(const i2c_bus&) = delete;
  i2c_bus& operator=(const i2c_bus&) = delete;

  i2c_bus(i2c_bus&& other) noexcept
      : handle_{other.handle_}, config_{other.config_} {
    other.handle_.Instance = nullptr;
  }

  i2c_bus& operator=(i2c_bus&& other) noexcept {
    if (this != &other) {
      if (handle_.Instance) HAL_I2C_DeInit(&handle_);
      handle_ = other.handle_;
      config_ = other.config_;
      other.handle_.Instance = nullptr;
    }
    return *this;
  }

  result<usize> write(u8 addr, std::span<const u8> data,
                      u32 timeout_ms = 1000) {
    auto status = HAL_I2C_Master_Transmit(
        &handle_, static_cast<u16>(addr << 1), const_cast<u8*>(data.data()),
        static_cast<u16>(data.size()), timeout_ms);
    if (status != HAL_OK) {
      return fail(error_code::io_error, "i2c write failed");
    }
    return ok(data.size());
  }

  result<usize> read(u8 addr, std::span<u8> buf, u32 timeout_ms = 1000) {
    auto status = HAL_I2C_Master_Receive(
        &handle_, static_cast<u16>(addr << 1), buf.data(),
        static_cast<u16>(buf.size()), timeout_ms);
    if (status != HAL_OK) {
      return fail(error_code::io_error, "i2c read failed");
    }
    return ok(buf.size());
  }

  result<usize> write_read(u8 addr, std::span<const u8> tx, std::span<u8> rx,
                           u32 timeout_ms = 1000) {
    auto w = write(addr, tx, timeout_ms);
    if (!w) return std::unexpected(w.error());
    return read(addr, rx, timeout_ms);
  }

  I2C_HandleTypeDef* handle() { return &handle_; }
  const i2c_config& config() const { return config_; }

 private:
  void init_gpio() {
    GPIO_InitTypeDef gpio{};
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = config_.af;

    if (config_.sda_port) {
      enable_gpio_clock(config_.sda_port);
      gpio.Pin = config_.sda_pin;
      HAL_GPIO_Init(config_.sda_port, &gpio);
    }
    if (config_.scl_port) {
      enable_gpio_clock(config_.scl_port);
      gpio.Pin = config_.scl_pin;
      HAL_GPIO_Init(config_.scl_port, &gpio);
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
  }

  static void enable_i2c_clock(I2C_TypeDef* inst) {
    if (inst == I2C1)
      __HAL_RCC_I2C1_CLK_ENABLE();
    else if (inst == I2C2)
      __HAL_RCC_I2C2_CLK_ENABLE();
    else if (inst == I2C3)
      __HAL_RCC_I2C3_CLK_ENABLE();
  }

  I2C_HandleTypeDef handle_{};
  i2c_config config_;
};

}  // namespace jstm::hal
