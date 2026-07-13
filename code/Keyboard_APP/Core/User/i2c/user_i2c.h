/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_I2C_H__
#define __USER_I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/**
 * @brief Macro to suppress unused parameter warnings.
 *        用于抑制未使用参数的编译警告的宏。
 */
#define UNUSED(X) (void)X

/**
 * @brief I2C receive/transmit buffer length.
 *        I2C 接收/发送缓冲区长度。
 */
#define I2C_RECEIVE_BUFFER_LEN 600

/**
 * @brief I2C timeout counter.
 *        I2C 超时计数器。
 * @note  This counter is incremented in the I2C interrupt handler
 *        to detect abnormal I2C bus conditions.
 *        该计数器在 I2C 中断中递增，
 *        用于检测 I2C 总线异常情况。
 */
extern volatile uint32_t i2c_timeout_counter;

/**
 * @brief I2C STOP timeout flag.
 *        I2C STOP 超时标志。
 * @note  This flag indicates that a STOP condition timeout
 *        monitoring is in progress.
 *        该标志表示正在进行 STOP 条件超时监测。
 */
extern volatile uint32_t i2c_stop_timeout_flag;

/**
 * @brief I2C STOP timeout counter.
 *        I2C STOP 超时计数器。
 * @note  Used together with i2c_stop_timeout_flag to detect
 *        missing STOP conditions.
 *        与 i2c_stop_timeout_flag 配合使用，
 *        用于检测 STOP 条件丢失情况。
 */
extern volatile uint32_t i2c_stop_timeout_counter;

/**
 * @brief Initialize I2C2 peripheral in slave mode.
 *        初始化 I2C2 外设（从机模式）。
 * @note  This function configures GPIO, clock, interrupts,
 *        and I2C timing parameters.
 *        该函数配置 GPIO、时钟、中断以及 I2C 时序参数。
 *
 * @retval None
 */
void user_i2c_init(void);

/**
 * @brief Enable I2C2 related interrupts.
 *        使能 I2C2 相关中断。
 * @note  Enables address match, NACK, error and STOP interrupts.
 *        使能地址匹配、NACK、错误以及 STOP 中断。
 *
 * @retval None
 */
void i2c2_it_enable(void);

/**
 * @brief Disable I2C2 related interrupts.
 *        禁用 I2C2 相关中断。
 * @note  Disables address match, NACK, error and STOP interrupts.
 *        禁用地址匹配、NACK、错误以及 STOP 中断。
 *
 * @retval None
 */
void i2c2_it_disable(void);

/**
 * @brief Set I2C slave transmit buffer.
 *        设置 I2C 从机发送数据缓冲区。
 * @note  Data will be transmitted to the I2C master
 *        during a read operation.
 *        在主机读操作时，
 *        从机将发送该缓冲区中的数据。
 *
 * @param tx_ptr Pointer to transmit data buffer.
 *               发送数据缓冲区指针。
 * @param len    Length of transmit data.
 *               发送数据长度。
 *
 * @retval None
 */
void i2c2_set_send_data(uint8_t *tx_ptr, uint16_t len);

/**
 * @brief Set I2C slave address.
 *        设置 I2C 从机地址。
 * @note  The address should be a 7-bit I2C address.
 *        该地址应为 7 位 I2C 地址。
 *
 * @param addr I2C slave address (7-bit).
 *             I2C 从机地址（7 位）。
 *
 * @retval None
 */
void set_i2c_slave_address(uint8_t addr);

#ifdef __cplusplus
}
#endif

#endif /* __USER_I2C_H__ */