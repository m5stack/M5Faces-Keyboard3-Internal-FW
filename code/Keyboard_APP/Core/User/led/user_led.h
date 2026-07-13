/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_LED_H__
#define __USER_LED_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define LED_L_1 HAL_GPIO_WritePin(GPIOB, LED1_Pin, GPIO_PIN_SET)
#define LED_R_1 HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET)
#define LED_L_0 HAL_GPIO_WritePin(GPIOB, LED1_Pin, GPIO_PIN_RESET)
#define LED_R_0 HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET)

extern __IO uint8_t led_mode;

void led_blink(void);

void led_update(void);

#ifdef __cplusplus
}
#endif

#endif /* __USER_LED_H__ */
