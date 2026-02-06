#ifndef STM32F7XX_HAL_CONF_H
#define STM32F7XX_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(HSE_VALUE)
#define HSE_VALUE ((uint32_t)8000000U) /* 8 MHz from ST-Link MCO */
#endif

#if !defined(HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT ((uint32_t)100U)
#endif

#if !defined(HSI_VALUE)
#define HSI_VALUE ((uint32_t)16000000U)
#endif

#if !defined(LSI_VALUE)
#define LSI_VALUE ((uint32_t)32000U)
#endif

#if !defined(LSE_VALUE)
#define LSE_VALUE ((uint32_t)32768U)
#endif

#if !defined(LSE_STARTUP_TIMEOUT)
#define LSE_STARTUP_TIMEOUT ((uint32_t)5000U)
#endif

#if !defined(EXTERNAL_CLOCK_VALUE)
#define EXTERNAL_CLOCK_VALUE ((uint32_t)12288000U)
#endif

#define HAL_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_ETH_MODULE_ENABLED
#define HAL_SRAM_MODULE_ENABLED

#define TICK_INT_PRIORITY ((uint32_t)0x0FU)
#define USE_RTOS 0U
#define PREFETCH_ENABLE 1U
#define ART_ACCELERATOR_ENABLE 1U
#define INSTRUCTION_CACHE_ENABLE 1U
#define DATA_CACHE_ENABLE 1U

#define ETH_MAC_ADDR0 ((uint8_t)0x00)
#define ETH_MAC_ADDR1 ((uint8_t)0x80)
#define ETH_MAC_ADDR2 ((uint8_t)0xE1)
#define ETH_MAC_ADDR3 ((uint8_t)0x00)
#define ETH_MAC_ADDR4 ((uint8_t)0x00)
#define ETH_MAC_ADDR5 ((uint8_t)0x00)

#define LAN8742A_PHY_ADDRESS 0x00U

#define PHY_READ_TO 0x0000FFFFU
#define PHY_WRITE_TO 0x0000FFFFU

#define PHY_BCR ((uint16_t)0x0000)
#define PHY_BSR ((uint16_t)0x0001)
#define PHY_RESET ((uint16_t)0x8000)
#define PHY_LOOPBACK ((uint16_t)0x4000)
#define PHY_FULLDUPLEX_100M ((uint16_t)0x2100)
#define PHY_HALFDUPLEX_100M ((uint16_t)0x2000)
#define PHY_FULLDUPLEX_10M ((uint16_t)0x0100)
#define PHY_HALFDUPLEX_10M ((uint16_t)0x0000)
#define PHY_AUTONEGOTIATION ((uint16_t)0x1000)
#define PHY_RESTART_AUTONEGOTIATION ((uint16_t)0x0200)
#define PHY_POWERDOWN ((uint16_t)0x0800)
#define PHY_ISOLATE ((uint16_t)0x0400)
#define PHY_AUTONEGO_COMPLETE ((uint16_t)0x0020)
#define PHY_LINKED_STATUS ((uint16_t)0x0004)
#define PHY_JABBER_DETECTION ((uint16_t)0x0002)

#define PHY_SR ((uint16_t)0x001F)
#define PHY_SPEED_STATUS ((uint16_t)0x0004)
#define PHY_DUPLEX_STATUS ((uint16_t)0x0010)

#if !defined(ETH_MACCR_CSTF)
#define ETH_MACCR_CSTF ((uint32_t)0x02000000)
#endif

#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32f7xx_hal_rcc.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32f7xx_hal_gpio.h"
#endif

#ifdef HAL_DMA_MODULE_ENABLED
#include "stm32f7xx_hal_dma.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32f7xx_hal_cortex.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32f7xx_hal_flash.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
#include "stm32f7xx_hal_pwr.h"
#include "stm32f7xx_hal_pwr_ex.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
#include "stm32f7xx_hal_uart.h"
#endif

#ifdef HAL_SPI_MODULE_ENABLED
#include "stm32f7xx_hal_spi.h"
#endif

#ifdef HAL_I2C_MODULE_ENABLED
#include "stm32f7xx_hal_i2c.h"
#endif

#ifdef HAL_TIM_MODULE_ENABLED
#include "stm32f7xx_hal_tim.h"
#endif

#ifdef HAL_ETH_MODULE_ENABLED
#include "stm32f7xx_hal_eth.h"
#endif

#ifdef HAL_SRAM_MODULE_ENABLED
#include "stm32f7xx_hal_sram.h"
#endif

/* #define USE_FULL_ASSERT  1U */

#ifdef USE_FULL_ASSERT
#define assert_param(expr) \
  ((expr) ? (void)0U : assert_failed((uint8_t*)__FILE__, __LINE__))
void assert_failed(uint8_t* file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* STM32F7XX_HAL_CONF_H */
