#include "stm32f7xx_it.h"

#include "stm32f7xx_hal.h"

void NMI_Handler(void) {}

void HardFault_Handler(void) {
  while (1) {
  }
}

void MemManage_Handler(void) {
  while (1) {
  }
}

void BusFault_Handler(void) {
  while (1) {
  }
}

void UsageFault_Handler(void) {
  while (1) {
  }
}

void DebugMon_Handler(void) {}

__attribute__((weak)) void SVC_Handler(void) {}

__attribute__((weak)) void PendSV_Handler(void) {}

__attribute__((weak)) void SysTick_Callback(void) {}

void SysTick_Handler(void) {
  HAL_IncTick();
  SysTick_Callback();
}
