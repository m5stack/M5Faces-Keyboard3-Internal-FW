/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_I2C_REG_H__
#define __USER_I2C_REG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define DEVICE_ID_REG_ADDR                (0xD0)
#define UID_REG_ADDR_START                (0xE0)
#define UID_REG_ADDR_END                  (0xEB)
#define UID_REG_LENGTH                    (12)
#define FACES_CMD_SET_OPERATION_MODE_ADDR (0xF0)
#define FACES_CMD_SET_LED_MODE_ADDR       (0xF1)
#define IAP_UPDATE_REG_ADDR               (0xFD)
#define FIRMWARE_VERSION_REG_ADDR         (0xFE)
#define I2C_ADDRESS_REG_ADDR              (0xFF)

#ifdef __cplusplus
}
#endif

#endif /* __USER_I2C_REG_H__ */
