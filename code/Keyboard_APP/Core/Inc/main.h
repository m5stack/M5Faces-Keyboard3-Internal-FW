/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

#include "stm32f0xx_ll_i2c.h"
#include "stm32f0xx_ll_iwdg.h"
#include "stm32f0xx_ll_system.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_exti.h"
#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_cortex.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_utils.h"
#include "stm32f0xx_ll_pwr.h"
#include "stm32f0xx_ll_dma.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BtnAA_Pin LL_GPIO_PIN_13
#define BtnAA_GPIO_Port GPIOC
#define BtnALT_Pin LL_GPIO_PIN_14
#define BtnALT_GPIO_Port GPIOC
#define BtnENTER_Pin LL_GPIO_PIN_15
#define BtnENTER_GPIO_Port GPIOC
#define BtnSYM_Pin LL_GPIO_PIN_0
#define BtnSYM_GPIO_Port GPIOF
#define BtnFN_Pin LL_GPIO_PIN_1
#define BtnFN_GPIO_Port GPIOF
#define A_Pin LL_GPIO_PIN_0
#define A_GPIO_Port GPIOA
#define B_Pin LL_GPIO_PIN_1
#define B_GPIO_Port GPIOA
#define C_Pin LL_GPIO_PIN_2
#define C_GPIO_Port GPIOA
#define D_Pin LL_GPIO_PIN_3
#define D_GPIO_Port GPIOA
#define E_Pin LL_GPIO_PIN_4
#define E_GPIO_Port GPIOA
#define F_Pin LL_GPIO_PIN_5
#define F_GPIO_Port GPIOA
#define G_Pin LL_GPIO_PIN_6
#define G_GPIO_Port GPIOA
#define H_Pin LL_GPIO_PIN_7
#define H_GPIO_Port GPIOA
#define OUTPUT_03_Pin LL_GPIO_PIN_0
#define OUTPUT_03_GPIO_Port GPIOB
#define OUTPUT_02_Pin LL_GPIO_PIN_1
#define OUTPUT_02_GPIO_Port GPIOB
#define OUTPUT_01_Pin LL_GPIO_PIN_2
#define OUTPUT_01_GPIO_Port GPIOB
#define I_Pin LL_GPIO_PIN_8
#define I_GPIO_Port GPIOA
#define J_Pin LL_GPIO_PIN_9
#define J_GPIO_Port GPIOA
#define INT_Pin LL_GPIO_PIN_15
#define INT_GPIO_Port GPIOA
#define LED1_Pin LL_GPIO_PIN_4
#define LED1_GPIO_Port GPIOB
#define LED2_Pin LL_GPIO_PIN_5
#define LED2_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */



/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
