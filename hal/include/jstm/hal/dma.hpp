#pragma once

#include <jstm/types.hpp>

#include "stm32f7xx_hal.h"

namespace jstm::hal {

class dma_channel {
 public:
  dma_channel(DMA_Stream_TypeDef* stream, u32 channel) {
    handle_.Instance = stream;
    handle_.Init.Channel = channel;
    handle_.Init.Direction = DMA_MEMORY_TO_PERIPH;
    handle_.Init.PeriphInc = DMA_PINC_DISABLE;
    handle_.Init.MemInc = DMA_MINC_ENABLE;
    handle_.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    handle_.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    handle_.Init.Mode = DMA_NORMAL;
    handle_.Init.Priority = DMA_PRIORITY_MEDIUM;
    handle_.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    HAL_DMA_Init(&handle_);
  }

  ~dma_channel() { HAL_DMA_DeInit(&handle_); }

  dma_channel(dma_channel&& other) noexcept : handle_{other.handle_} {
    other.handle_.Instance = nullptr;
  }

  dma_channel& operator=(dma_channel&& other) noexcept {
    if (this != &other) {
      if (handle_.Instance) HAL_DMA_DeInit(&handle_);
      handle_ = other.handle_;
      other.handle_.Instance = nullptr;
    }
    return *this;
  }

  dma_channel(const dma_channel&) = delete;
  dma_channel& operator=(const dma_channel&) = delete;

  void start(const void* src, void* dst, u32 length) {
    HAL_DMA_Start(&handle_, reinterpret_cast<u32>(src),
                  reinterpret_cast<u32>(dst), length);
  }

  void wait() {
    HAL_DMA_PollForTransfer(&handle_, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
  }

  bool busy() const { return handle_.State == HAL_DMA_STATE_BUSY; }

  DMA_HandleTypeDef* handle() { return &handle_; }

 private:
  DMA_HandleTypeDef handle_{};
};

}  // namespace jstm::hal
