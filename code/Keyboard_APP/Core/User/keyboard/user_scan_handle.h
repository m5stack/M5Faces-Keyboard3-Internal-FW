/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_SCAN_HANDLE_H__
#define __USER_SCAN_HANDLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* ============================================================
 * 编译期常量
 * ============================================================ */
#define DOUBLE_CLICK_THRESHOLD_MS (500U)  /**< 双击判定窗口 ms        */
#define BTN_DEBOUNCE_MS           (20U)   /**< 特殊键消抖时间 ms       */
#define MATRIX_DEBOUNCE_MS        (30U)   /**< 矩阵键消抖稳定时间 ms   */
#define MATRIX_SETTLE_US          (100U)  /**< 矩阵扫描建立延迟 us     */
#define DIRECT_SETTLE_MS          (2U)    /**< 直连模式建立延迟 ms     */
#define KEY_NONE                  (0xFFU) /**< 无效键值标志            */
#define SEND_DATA_LEN             (10U)   /**< send_data 缓冲区长度    */
#define OUTPUT_DATA_GROUPS        (4U)    /**< old_output_data 组数    */

/* ============================================================
 * 工作模式
 * ============================================================ */
typedef enum {
    FACES_OPERATION_MODE_NORMAL = 0,
    FACES_OPERATION_MODE_DIRECT = 1,
} FacesOperationMode;

/* ============================================================
 * 特殊按键 ID
 * ============================================================ */
typedef enum {
    BTN_AA    = 0,
    BTN_ALT   = 1,
    BTN_ENTER = 2,
    BTN_SYM   = 3,
    BTN_FN    = 4,
} BtnId;

/* ============================================================
 * 矩阵键消抖状态机
 *
 *  IDLE       → 无任何按键，等待新输入
 *  DEBOUNCING → 检测到单键，等待稳定
 *  TRIGGERED  → 键已触发上报，等待物理松开
 *  LOCKOUT    → 检测到多键或异常，封锁所有输入，
 *               必须等全部键松开后才恢复 IDLE
 * ============================================================ */
typedef enum {
    MATRIX_IDLE       = 0,
    MATRIX_DEBOUNCING = 1,
    MATRIX_TRIGGERED  = 2,
    MATRIX_LOCKOUT    = 3,
} MatrixState;

/**
 * @brief 设置矩阵扫描输出模式（1/2/3），范围外不操作。
 */
static inline void output_mode_set(uint8_t mode)
{
    /* 三路引脚电平：[0]=OUTPUT_01  [1]=OUTPUT_02  [2]=OUTPUT_03 */
    static const GPIO_PinState lut[3][3] = {
        {GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET}, /* mode 1 */
        {GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_SET}, /* mode 2 */
        {GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET}, /* mode 3 */
    };
    if (mode < 1U || mode > 3U) return;
    const GPIO_PinState *lv = lut[mode - 1U];
    HAL_GPIO_WritePin(GPIOB, OUTPUT_01_Pin, lv[0]);
    HAL_GPIO_WritePin(GPIOB, OUTPUT_02_Pin, lv[1]);
    HAL_GPIO_WritePin(GPIOB, OUTPUT_03_Pin, lv[2]);
}

/**
 * @brief 读取特殊按键电平（GPIO_PIN_RESET = 按下）。
 */
static inline GPIO_PinState btn_read(BtnId id)
{
    switch (id) {
        case BTN_AA:
            return HAL_GPIO_ReadPin(GPIOC, BtnAA_Pin);
        case BTN_ALT:
            return HAL_GPIO_ReadPin(GPIOC, BtnALT_Pin);
        case BTN_ENTER:
            return HAL_GPIO_ReadPin(GPIOC, BtnENTER_Pin);
        case BTN_SYM:
            return HAL_GPIO_ReadPin(GPIOF, BtnSYM_Pin);
        case BTN_FN:
            return HAL_GPIO_ReadPin(GPIOF, BtnFN_Pin);
        default:
            return GPIO_PIN_SET;
    }
}

/* ============================================================
 * 对外共享变量
 * 仅对 ISR 与主循环均访问的变量使用 volatile
 * ============================================================ */
extern volatile uint8_t send_data[SEND_DATA_LEN];             /**< I2C 发送缓冲（ISR+主循环）*/
extern volatile uint16_t old_output_data[OUTPUT_DATA_GROUPS]; /**< 上帧直连数据（主循环）*/
extern volatile uint8_t operation_mode;                       /**< 工作模式（可由 I2C 写入）  */
extern volatile uint8_t hadScaned;                            /**< 直连扫描变化标志（ISR+主）  */

/* ============================================================
 * 对外接口
 * ============================================================ */
void request_event(void);   /**< I2C 请求中断中调用 */
void keyboard_update(void); /**< 主循环中调用       */
void direct_mode_sync_baseline(void);
#ifdef __cplusplus
}
#endif
#endif /* __USER_SCAN_HANDLE_H__ */