/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_INT_H__
#define __USER_INT_H__

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IRQ_CLR HAL_GPIO_WritePin(INT_GPIO_Port, INT_Pin, GPIO_PIN_SET)
#define IRQ_SET HAL_GPIO_WritePin(INT_GPIO_Port, INT_Pin, GPIO_PIN_RESET)

#ifdef __cplusplus
}
#endif

#endif /* __USER_INT_H__ */
