#include "stm32f7xx.h"

#if !defined(VECT_TAB_SRAM)
#define VECT_TAB_OFFSET 0x00U
#endif

uint32_t SystemCoreClock = 16000000U;

const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                   1, 2, 3, 4, 6, 7, 8, 9};

const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};

void SystemInit(void) {
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
  SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
#endif

#if defined(VECT_TAB_SRAM)
  SCB->VTOR = RAMDTCM_BASE | VECT_TAB_OFFSET;
#else
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;
#endif
}
