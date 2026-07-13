/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_i2c_callback.h"
#include "string.h"
#include "user_i2c_reg.h"
#include "user_i2c.h"
#include "user_scan_handle.h"
#include "user_led.h"
#include "user_sys.h"

__IO uint8_t i2c_addr_reg            = 0;
__IO uint8_t tx_prepared             = 0;
static uint8_t g_uid[UID_REG_LENGTH] = {0};

static __IO uint32_t i2c_stop_timeout_delay = 0;

void read_uid(void)
{
    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    uint32_t uid2 = HAL_GetUIDw2();

    memcpy(&g_uid[0], &uid0, sizeof(uid0));
    memcpy(&g_uid[4], &uid1, sizeof(uid1));
    memcpy(&g_uid[8], &uid2, sizeof(uid2));
}

void Slave_Complete_Callback(uint8_t *rx_data, uint16_t len)
{
    uint8_t rx_buf[64];
    uint8_t tx_buf[64];
    uint8_t rx_mark[64] = {0};

    if (len == 1) {
        if (rx_data[0] == DEVICE_ID_REG_ADDR) {
            tx_buf[0] = DEVICE_ID;
            i2c2_set_send_data((uint8_t *)&tx_buf[0], 1);
            tx_prepared = 1;
        } else if ((rx_data[0] >= UID_REG_ADDR_START) && (rx_data[0] <= UID_REG_ADDR_END)) {
            uint8_t uid_offset = rx_data[0] - UID_REG_ADDR_START;
            i2c2_set_send_data(&g_uid[uid_offset], UID_REG_LENGTH - uid_offset);
            tx_prepared = 1;
        } else if ((rx_data[0] >= FACES_CMD_SET_OPERATION_MODE_ADDR) && (rx_data[0] <= FACES_CMD_SET_LED_MODE_ADDR)) {
            tx_buf[0]   = operation_mode;
            tx_buf[1]   = led_mode;
            tx_prepared = 1;
            i2c2_set_send_data((uint8_t *)&tx_buf[rx_data[0] - FACES_CMD_SET_OPERATION_MODE_ADDR],
                               FACES_CMD_SET_LED_MODE_ADDR - rx_data[0] + 1);
        } else if ((rx_data[0] >= FIRMWARE_VERSION_REG_ADDR) && (rx_data[0] <= I2C_ADDRESS_REG_ADDR)) {
            tx_buf[0]   = FIRMWARE_VERSION;
            tx_buf[1]   = i2c_addr_reg;
            tx_prepared = 1;
            i2c2_set_send_data((uint8_t *)&tx_buf[rx_data[0] - FIRMWARE_VERSION_REG_ADDR],
                               I2C_ADDRESS_REG_ADDR - rx_data[0] + 1);
        }
    } else if (len > 1) {
        if (rx_data[0] == FACES_CMD_SET_OPERATION_MODE_ADDR && len == 2) {
            operation_mode = rx_data[1];
        } else if (rx_data[0] == FACES_CMD_SET_LED_MODE_ADDR && len == 2) {
            led_mode = rx_data[1];
        } else if (rx_data[0] == IAP_UPDATE_REG_ADDR && len == 2) {
            if (rx_data[1]) {
                NVIC_SystemReset();
            }
        } else if (rx_data[0] == I2C_ADDRESS_REG_ADDR && len == 2) {
            if (rx_data[1] >= I2C_ADDR_REG_MIN && rx_data[1] <= I2C_ADDR_REG_MAX) {
                if (i2c_addr_reg != rx_data[1]) {
                    i2c_addr_reg = rx_data[1];
                    set_i2c_addr(rx_data[1]);
                    user_i2c_init();
                    i2c2_it_enable();
                }
            }
        }
    }
}

/**
 * @brief Handler for I2C timeout events to recover the bus.
 *        I2C 超时事件处理句柄，用于恢复总线。
 * @note  Monitors stop condition timeout and re-initializes I2C if necessary.
 *        监控停止条件超时并在必要时重新初始化 I2C。
 *
 * @param None
 * @retval None
 */
void i2c_timeout_handler(void)
{
    i2c_timeout_counter = 0;
    if (i2c_stop_timeout_flag) {
        if (i2c_stop_timeout_delay < HAL_GetTick()) {
            i2c_stop_timeout_counter++;
            i2c_stop_timeout_delay = HAL_GetTick() + 10;
        }
    }
    // If timeout counter exceeds limit, reset I2C peripheral / 若超时计数超过限制，重置 I2C 外设
    if (i2c_stop_timeout_counter > 50) {
        LL_I2C_DeInit(I2C2);
        LL_I2C_DisableAutoEndMode(I2C2);
        LL_I2C_Disable(I2C2);
        LL_I2C_DisableIT_ADDR(I2C2);
        user_i2c_init();   // Recovery initialization / 恢复初始化
        i2c2_it_enable();  // Re-enable interrupts / 重新使能中断
        HAL_Delay(500);
    }
}
