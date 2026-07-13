/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "user_flash.h"
#include "stm32f0xx_hal_flash_ex.h"
#include "user_i2c_callback.h"
#include "user_sys.h"

/* --------------------------------------------------------------------------
 * Static Helper Functions
 * --------------------------------------------------------------------------*/

/**
 * @brief  Modify a single byte within a 64-bit data word.
 * @param  data       Pointer to 64-bit data.
 * @param  byte_index Target byte index (0 ~ 7).
 * @param  new_value  New byte value.
 */
static void set_byte_in_uint64(uint64_t *data, uint8_t byte_index, uint8_t new_value)
{
    *data &= ~((uint64_t)0xFF << (byte_index * 8U));
    *data |= ((uint64_t)new_value << (byte_index * 8U));
}

/**
 * @brief  Read a double word (64-bit) from flash memory.
 * @param  address Flash address.
 * @retval 64-bit value read from flash.
 */
static uint64_t flash_read_double_word(uint32_t address)
{
    return *((__IO uint64_t *)address);
}

/**
 * @brief  Erase a single flash page.
 * @param  page_address Page start address.
 * @retval true  Page erased successfully.
 * @retval false Erase operation failed.
 */
static bool flash_erase_page(uint32_t page_address)
{
    FLASH_EraseInitTypeDef erase_init = {.TypeErase = FLASH_TYPEERASE_PAGES, .PageAddress = page_address, .NbPages = 1};
    uint32_t page_error               = 0U;

    HAL_FLASH_Unlock();
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    HAL_FLASH_Lock();

    return (status == HAL_OK);
}

/**
 * @brief  Program a 64-bit double word into flash memory.
 * @param  address Target address.
 * @param  data    64-bit data to write.
 * @retval true  Write succeeded.
 * @retval false Write failed.
 */
static bool flash_write_double_word(uint32_t address, uint64_t data)
{
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data);
    HAL_FLASH_Lock();

    return (status == HAL_OK);
}

/**
 * @brief  Store an I2C address into flash memory.
 * @param  data New I2C address value.
 * @retval true  Operation succeeded and verification passed.
 * @retval false Operation failed.
 */
bool set_i2c_addr(uint8_t data)
{
    uint64_t temp = flash_read_double_word(STM32F0XX_FLASH_PAGE61_ADDR);
    set_byte_in_uint64(&temp, I2C_ADDR_FLASH_OFFSET, data);

    __disable_irq();
    bool erased  = flash_erase_page(STM32F0XX_FLASH_PAGE61_ADDR);
    bool written = false;

    if (erased) {
        written = flash_write_double_word(STM32F0XX_FLASH_PAGE61_ADDR, temp);
    }
    __enable_irq();

    if (written && (data == get_i2c_addr())) {
        return true;
    }
    return false;
}

/**
 * @brief  Retrieve the stored I2C address from flash memory.
 * @retval Current I2C address.
 */
uint8_t get_i2c_addr(void)
{
    return *((__IO uint8_t *)I2C_ADDR_FLASH_LOCATION);
}
