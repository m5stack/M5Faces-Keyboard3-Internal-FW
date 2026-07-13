/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_I2C_CALLBACK_H__
#define __USER_I2C_CALLBACK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

extern __IO uint8_t i2c_addr_reg;
extern __IO uint8_t tx_prepared;

void read_uid(void);

/**
 * @brief Callback function executed when I2C slave transfer is complete.
 *        I2C 从机传输完成时执行的回调函数。
 *
 * @param rx_data Pointer to the received data buffer.
 *                指向接收数据缓冲区的指针。
 * @param len Length of the received data.
 *            接收数据的长度。
 * @retval None
 */
void Slave_Complete_Callback(uint8_t *rx_data, uint16_t len);

/**
 * @brief Handler for I2C timeout events to recover the bus.
 *        I2C 超时事件处理句柄，用于恢复总线。
 *
 * @param None
 * @retval None
 */
void i2c_timeout_handler(void);

#ifdef __cplusplus
}
#endif

#endif /*__USER_I2C_CALLBACK_H__ */
