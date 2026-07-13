/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_led.h"

__IO uint8_t led_mode = 0;

void led_blink(void)
{
    for (int i = 0; i < 5; i++) {
        LED_L_1;
        LED_R_0;
        HAL_Delay(i * 30);
        LED_R_1;
        LED_L_0;
        HAL_Delay(i * 30);
    }
    LED_R_0;
    LED_L_0;
}

void led_update(void)
{
    uint32_t tick = HAL_GetTick();

    switch (led_mode) {
        case 0:
            LED_L_0;
            LED_R_0;
            break;
        case 1:
            LED_L_1;
            LED_R_0;
            break;
        case 2:
            // 左灯慢闪：每 500ms 翻转一次状态
            if ((tick / 500) % 2 == 1) {
                LED_L_0;
                LED_R_0;
            } else {
                LED_L_1;
                LED_R_0;
            }
            break;
        case 3:
            // 左灯快闪：每 150ms 翻转一次状态
            if ((tick / 150) % 2 == 1) {
                LED_L_0;
                LED_R_0;
            } else {
                LED_L_1;
                LED_R_0;
            }
            break;
        case 4:
            LED_R_1;
            LED_L_0;
            break;
        case 5:
            // 右灯慢闪
            if ((tick / 500) % 2 == 1) {
                LED_R_0;
                LED_L_0;
            } else {
                LED_R_1;
                LED_L_0;
            }
            break;
        case 6:
            // 右灯快闪
            if ((tick / 150) % 2 == 1) {
                LED_R_0;
                LED_L_0;
            } else {
                LED_R_1;
                LED_L_0;
            }
            break;
        case 7:
            // 左右交替慢闪：周期 500ms
            if ((tick / 500) % 2 == 1) {
                LED_L_1;
                LED_R_0;
            } else {
                LED_R_1;
                LED_L_0;
            }
            break;
        case 8:
            // 左右交替快闪：周期 200ms
            if ((tick / 200) % 2 == 1) {
                LED_L_1;
                LED_R_0;
            } else {
                LED_R_1;
                LED_L_0;
            }
            break;
        default:
            // 保持原有的 Bitmask 直接控制逻辑
            if (led_mode & 0x80) {
                if ((led_mode >> 4) & 1) {
                    LED_L_1;
                } else {
                    LED_L_0;
                }
                if ((led_mode >> 5) & 1) {
                    LED_R_1;
                } else {
                    LED_R_0;
                }
            }
    }
}
