/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_i2c.h"
#include <stdio.h>
#include <string.h>
#include "i2c.h"
#include "user_i2c_callback.h"
#include "user_scan_handle.h"

/**
 * @brief I2C slave receive buffer.
 *        I2C 从机接收缓冲区。
 */
__IO uint8_t aReceiveBuffer[I2C_RECEIVE_BUFFER_LEN] = {0};

/**
 * @brief I2C slave transmit buffer.
 *        I2C 从机发送缓冲区。
 */
__IO uint8_t tx_buffer[I2C_RECEIVE_BUFFER_LEN] = {0};

/**
 * @brief I2C receive buffer index.
 *        I2C 接收缓冲区索引。
 */
__IO uint16_t ubReceiveIndex = 0;

/**
 * @brief Pointer to slave transmit buffer.
 *        指向从机发送缓冲区的指针。
 */
uint8_t *pSlaveTransmitBuffer = 0;

/**
 * @brief I2C slave address (left-shifted).
 *        I2C 从机地址（左移后的格式）。
 */
volatile uint8_t i2c_addr = 0;

/**
 * @brief I2C transmit buffer index.
 *        I2C 发送缓冲区索引。
 */
volatile uint16_t tx_buffer_index = 0;

/**
 * @brief I2C transmit data length.
 *        I2C 发送数据长度。
 */
volatile uint16_t tx_len = 0;

/**
 * @brief I2C timeout counter.
 *        I2C 超时计数器。
 */
volatile uint32_t i2c_timeout_counter = 0;

/**
 * @brief I2C STOP timeout flag.
 *        I2C STOP 超时标志。
 */
volatile uint32_t i2c_stop_timeout_flag = 0;

/**
 * @brief I2C STOP timeout counter.
 *        I2C STOP 超时计数器。
 */
volatile uint32_t i2c_stop_timeout_counter = 0;

/**
 * @brief Initialize I2C2 peripheral in slave mode.
 *        初始化 I2C2 外设（从机模式）。
 *
 * @retval None
 */
void user_i2c_init(void)
{
    LL_I2C_InitTypeDef I2C_InitStruct   = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIO clock */
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

    /**
     * @note I2C2 GPIO Configuration
     *       PB10 -> I2C2_SCL
     *       PB11 -> I2C2_SDA
     */
    GPIO_InitStruct.Pin        = LL_GPIO_PIN_10;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate  = LL_GPIO_AF_1;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Enable I2C2 peripheral clock */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C2);

    /* Configure I2C2 interrupt */
    NVIC_SetPriority(I2C2_IRQn, 0);
    NVIC_EnableIRQ(I2C2_IRQn);

    /* I2C configuration */
    LL_I2C_DisableOwnAddress2(I2C2);
    LL_I2C_DisableGeneralCall(I2C2);
    LL_I2C_EnableClockStretching(I2C2);

    I2C_InitStruct.PeripheralMode  = LL_I2C_MODE_I2C;
    I2C_InitStruct.Timing          = 0x00300617;
    I2C_InitStruct.AnalogFilter    = LL_I2C_ANALOGFILTER_ENABLE;
    I2C_InitStruct.DigitalFilter   = 0;
    I2C_InitStruct.OwnAddress1     = i2c_addr_reg << 1;
    I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
    I2C_InitStruct.OwnAddrSize     = LL_I2C_OWNADDRESS1_7BIT;

    LL_I2C_Init(I2C2, &I2C_InitStruct);
    LL_I2C_EnableAutoEndMode(I2C2);
    LL_I2C_SetOwnAddress2(I2C2, 0, LL_I2C_OWNADDRESS2_NOMASK);

    /* Set slave address */
    set_i2c_slave_address(i2c_addr_reg);
}

/**
 * @brief Set I2C slave address.
 *        设置 I2C 从机地址。
 *
 * @param addr 7-bit I2C slave address.
 *             7 位 I2C 从机地址。
 *
 * @retval None
 */
void set_i2c_slave_address(uint8_t addr)
{
    i2c_addr = (addr << 1);
}

/**
 * @brief Slave transfer complete callback (weak implementation).
 *        从机传输完成回调函数（弱定义）。
 * @note  User can override this function to process received data.
 *        用户可重写该函数以处理接收到的数据。
 *
 * @param rx_data Pointer to received data buffer.
 *                接收数据缓冲区指针。
 * @param len     Length of received data.
 *                接收数据长度。
 *
 * @retval None
 */
__weak void Slave_Complete_Callback(uint8_t *rx_data, uint16_t len)
{
    UNUSED(rx_data);
    UNUSED(len);
}

/**
 * @brief Enable I2C2 interrupts.
 *        使能 I2C2 中断。
 *
 * @retval None
 */
void i2c2_it_enable(void)
{
    LL_I2C_Enable(I2C2);
    LL_I2C_EnableIT_ADDR(I2C2);
    LL_I2C_EnableIT_NACK(I2C2);
    LL_I2C_EnableIT_ERR(I2C2);
    LL_I2C_EnableIT_STOP(I2C2);
}

/**
 * @brief Disable I2C2 interrupts.
 *        禁用 I2C2 中断。
 *
 * @retval None
 */
void i2c2_it_disable(void)
{
    LL_I2C_DisableIT_ADDR(I2C2);
    LL_I2C_DisableIT_NACK(I2C2);
    LL_I2C_DisableIT_ERR(I2C2);
    LL_I2C_DisableIT_STOP(I2C2);
}

/**
 * @brief I2C error callback.
 *        I2C 错误回调函数。
 *
 * @retval None
 */
void Error_Callback(void)
{
    /* User-defined error handling can be added here */
}

/**
 * @brief Set I2C slave transmit data.
 *        设置 I2C 从机发送数据。
 *
 * @param tx_ptr Pointer to transmit buffer.
 *               发送缓冲区指针。
 * @param len    Length of transmit data.
 *               发送数据长度。
 *
 * @retval None
 */
void i2c2_set_send_data(uint8_t *tx_ptr, uint16_t len)
{
    if (len > I2C_RECEIVE_BUFFER_LEN) {
        len = I2C_RECEIVE_BUFFER_LEN;
    }
    if (len == 0 || tx_ptr == NULL) {
        return;
    }

    memcpy((void *)tx_buffer, tx_ptr, len);
    tx_buffer_index = 0;
    tx_len          = len;
}

/**
 * @brief Slave reception callback.
 *        从机接收数据回调函数。
 *
 * @retval None
 */
void Slave_Reception_Callback(void)
{
    if (ubReceiveIndex >= I2C_RECEIVE_BUFFER_LEN) {
        ubReceiveIndex = 0;
    }
    aReceiveBuffer[ubReceiveIndex++] = LL_I2C_ReceiveData8(I2C2);
}

/**
 * @brief Slave ready-to-transmit callback.
 *        从机准备发送数据回调函数。
 *
 * @retval None
 */
void Slave_Ready_To_Transmit_Callback(void)
{
    LL_I2C_TransmitData8(I2C2, tx_buffer[tx_buffer_index]);
    tx_buffer_index++;

    if (tx_buffer_index >= tx_len) {
        tx_buffer_index = 0;
    }
}

/**
 * @brief I2C2 interrupt handler.
 *        I2C2 中断服务函数。
 *
 * @retval None
 */
void I2C2_IRQHandler(void)
{
    /* I2C timeout monitoring */
    i2c_timeout_counter++;
    if (i2c_timeout_counter > 12000) {
        LL_I2C_DeInit(I2C2);
        LL_I2C_DisableAutoEndMode(I2C2);
        LL_I2C_Disable(I2C2);
        LL_I2C_DisableIT_ADDR(I2C2);
        user_i2c_init();
        i2c2_it_enable();
        i2c_timeout_counter = 0;
    }

    /* Address matched */
    if (LL_I2C_IsActiveFlag_ADDR(I2C2)) {
        if (LL_I2C_GetAddressMatchCode(I2C2) == i2c_addr) {
            if (ubReceiveIndex) {
                i2c2_it_disable();
                Slave_Complete_Callback((uint8_t *)aReceiveBuffer, ubReceiveIndex);
                ubReceiveIndex = 0;
                i2c2_it_enable();
            }

            if (LL_I2C_GetTransferDirection(I2C2) == LL_I2C_DIRECTION_WRITE) {
                LL_I2C_ClearFlag_ADDR(I2C2);
                LL_I2C_EnableIT_RX(I2C2);
                i2c_stop_timeout_flag = 1;
            } else if (LL_I2C_GetTransferDirection(I2C2) == LL_I2C_DIRECTION_READ) {
                if (!tx_prepared) {
                    request_event();
                }
                tx_prepared = 0;
                LL_I2C_ClearFlag_ADDR(I2C2);
                LL_I2C_EnableIT_TX(I2C2);
            } else {
                LL_I2C_ClearFlag_ADDR(I2C2);
                Error_Callback();
            }
        } else {
            LL_I2C_ClearFlag_ADDR(I2C2);
            Error_Callback();
        }
    }
    /* NACK received */
    else if (LL_I2C_IsActiveFlag_NACK(I2C2)) {
        LL_I2C_ClearFlag_NACK(I2C2);
    }
    /* Transmit interrupt */
    else if (LL_I2C_IsActiveFlag_TXIS(I2C2)) {
        Slave_Ready_To_Transmit_Callback();
    }
    /* Receive interrupt */
    else if (LL_I2C_IsActiveFlag_RXNE(I2C2)) {
        Slave_Reception_Callback();
    }
    /* STOP detected */
    else if (LL_I2C_IsActiveFlag_STOP(I2C2)) {
        LL_I2C_ClearFlag_STOP(I2C2);

        if (!LL_I2C_IsActiveFlag_TXE(I2C2)) {
            LL_I2C_ClearFlag_TXE(I2C2);
        }

        i2c2_it_disable();
        Slave_Complete_Callback((uint8_t *)aReceiveBuffer, ubReceiveIndex);
        ubReceiveIndex = 0;
        i2c2_it_enable();

        i2c_stop_timeout_flag    = 0;
        i2c_stop_timeout_counter = 0;
    } else {
        Error_Callback();
    }
}
