#pragma once

#include <jstm/result.hpp>
#include <jstm/types.hpp>
#include <span>

#include "stm32f7xx_hal.h"

namespace jstm::hal {

struct spi_config {
  u32 baudrate_prescaler = SPI_BAUDRATEPRESCALER_16;
  u32 cpol = SPI_POLARITY_LOW;
  u32 cpha = SPI_PHASE_1EDGE;
  GPIO_TypeDef* sck_port = nullptr;
  u16 sck_pin = 0;
  GPIO_TypeDef* mosi_port = nullptr;
  u16 mosi_pin = 0;
  GPIO_TypeDef* miso_port = nullptr;
  u16 miso_pin = 0;
  u8 af = 0;
};

class spi_bus {
 public:
  spi_bus(SPI_TypeDef* instance, spi_config cfg) : config_{cfg} {
    handle_.Instance = instance;
    handle_.Init.Mode = SPI_MODE_MASTER;
    handle_.Init.Direction = SPI_DIRECTION_2LINES;
    handle_.Init.DataSize = SPI_DATASIZE_8BIT;
    handle_.Init.CLKPolarity = config_.cpol;
    handle_.Init.CLKPhase = config_.cpha;
    handle_.Init.NSS = SPI_NSS_SOFT;
    handle_.Init.BaudRatePrescaler = config_.baudrate_prescaler;
    handle_.Init.FirstBit = SPI_FIRSTBIT_MSB;
    handle_.Init.TIMode = SPI_TIMODE_DISABLE;
    handle_.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    handle_.Init.CRCPolynomial = 7;

    init_gpio();
    enable_spi_clock(instance);
    HAL_SPI_Init(&handle_);
  }

  ~spi_bus() { HAL_SPI_DeInit(&handle_); }

  spi_bus(const spi_bus&) = delete;
  spi_bus& operator=(const spi_bus&) = delete;

  spi_bus(spi_bus&& other) noexcept
      : handle_{other.handle_}, config_{other.config_} {
    other.handle_.Instance = nullptr;
  }

  spi_bus& operator=(spi_bus&& other) noexcept {
    if (this != &other) {
      if (handle_.Instance) HAL_SPI_DeInit(&handle_);
      handle_ = other.handle_;
      config_ = other.config_;
      other.handle_.Instance = nullptr;
    }
    return *this;
  }

  result<usize> write(std::span<const u8> data, u32 timeout_ms = 1000) {
    auto status = HAL_SPI_Transmit(&handle_, const_cast<u8*>(data.data()),
                                   static_cast<u16>(data.size()), timeout_ms);
    if (status != HAL_OK) {
      return fail(error_code::io_error, "spi write failed");
    }
    return ok(data.size());
  }

  result<usize> transfer(std::span<const u8> tx, std::span<u8> rx,
                         u32 timeout_ms = 1000) {
    auto len = std::min(tx.size(), rx.size());
    auto status =
        HAL_SPI_TransmitReceive(&handle_, const_cast<u8*>(tx.data()), rx.data(),
                                static_cast<u16>(len), timeout_ms);
    if (status != HAL_OK) {
      return fail(error_code::io_error, "spi transfer failed");
    }
    return ok(len);
  }

  SPI_HandleTypeDef* handle() { return &handle_; }
  const spi_config& config() const { return config_; }

 private:
  void init_gpio() {
    GPIO_InitTypeDef gpio{};
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = config_.af;

    if (config_.sck_port) {
      enable_gpio_clock(config_.sck_port);
      gpio.Pin = config_.sck_pin;
      HAL_GPIO_Init(config_.sck_port, &gpio);
    }
    if (config_.mosi_port) {
      enable_gpio_clock(config_.mosi_port);
      gpio.Pin = config_.mosi_pin;
      HAL_GPIO_Init(config_.mosi_port, &gpio);
    }
    if (config_.miso_port) {
      enable_gpio_clock(config_.miso_port);
      gpio.Pin = config_.miso_pin;
      HAL_GPIO_Init(config_.miso_port, &gpio);
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

  static void enable_spi_clock(SPI_TypeDef* inst) {
    if (inst == SPI1)
      __HAL_RCC_SPI1_CLK_ENABLE();
    else if (inst == SPI2)
      __HAL_RCC_SPI2_CLK_ENABLE();
    else if (inst == SPI3)
      __HAL_RCC_SPI3_CLK_ENABLE();
    else if (inst == SPI4)
      __HAL_RCC_SPI4_CLK_ENABLE();
  }

  SPI_HandleTypeDef handle_{};
  spi_config config_;
};

}  // namespace jstm::hal
