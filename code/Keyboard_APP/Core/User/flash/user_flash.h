/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __USER_FLASH_H__
#define __USER_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stdbool.h"

/* Flash Page Definition -----------------------------------------------------*/
#define STM32F0XX_PAGE_SIZE          (0x400U) /* Flash page size: 1KB */
#define STM32F0XX_FLASH_BASE_ADDR    (0x08000000U)
#define STM32F0XX_FLASH_PAGE_ADDR(n) (STM32F0XX_FLASH_BASE_ADDR + ((n) * STM32F0XX_PAGE_SIZE))

#define STM32F0XX_FLASH_PAGE60_ADDR STM32F0XX_FLASH_PAGE_ADDR(60)
#define STM32F0XX_FLASH_PAGE61_ADDR STM32F0XX_FLASH_PAGE_ADDR(61)
#define STM32F0XX_FLASH_PAGE62_ADDR STM32F0XX_FLASH_PAGE_ADDR(62)
#define STM32F0XX_FLASH_PAGE63_ADDR STM32F0XX_FLASH_PAGE_ADDR(63)

/* User Data Definition ------------------------------------------------------*/
#define I2C_ADDR_FLASH_OFFSET   (0U)
#define I2C_ADDR_FLASH_LOCATION (STM32F0XX_FLASH_PAGE61_ADDR + I2C_ADDR_FLASH_OFFSET)

/* Public API ----------------------------------------------------------------*/

bool set_i2c_addr(uint8_t data);
uint8_t get_i2c_addr(void);

#ifdef __cplusplus
}
#endif

#endif /* __USER_FLASH_H__ */
