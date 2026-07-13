/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_SYS_H__
#define __USER_SYS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "stdbool.h"

#define APPLICATION_ADDRESS      (0x08001000)
#define FACES_TYPE_ID_CALCULATOR (1)
#define FACES_TYPE_ID_KEYBOARD   (2)
#define FACES_TYPE_ID_GAMEBOY    (3)
#define DEVICE_ID                (FACES_TYPE_ID_KEYBOARD)
#define FIRMWARE_VERSION         (0x03)
#define I2C_ADDR_REG_MIN         (0x08)
#define I2C_ADDR_REG_MAX         (0x77)
#define I2C_ADDR_REG_DEFAULT     (0x08)
#define FLASH_DEFAULT_DATA       (0xFF)

#ifdef __cplusplus
}
#endif

#endif /* __USER_SYS_H__ */
