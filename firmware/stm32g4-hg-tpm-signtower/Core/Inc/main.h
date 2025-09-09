/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "stm32g4xx_hal.h"

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
#define STATUS_LED_Pin GPIO_PIN_13
#define STATUS_LED_GPIO_Port GPIOC
#define LINK_LED_Pin GPIO_PIN_14
#define LINK_LED_GPIO_Port GPIOC
#define ERR_LED_Pin GPIO_PIN_15
#define ERR_LED_GPIO_Port GPIOC
#define ID1_Pin GPIO_PIN_5
#define ID1_GPIO_Port GPIOA
#define P_OUT7_Pin GPIO_PIN_7
#define P_OUT7_GPIO_Port GPIOA
#define P_OUT8_Pin GPIO_PIN_0
#define P_OUT8_GPIO_Port GPIOB
#define U3_TXD_Pin GPIO_PIN_10
#define U3_TXD_GPIO_Port GPIOB
#define U3_RXD_Pin GPIO_PIN_11
#define U3_RXD_GPIO_Port GPIOB
#define P_OUT4_Pin GPIO_PIN_12
#define P_OUT4_GPIO_Port GPIOB
#define P_OUT6_Pin GPIO_PIN_13
#define P_OUT6_GPIO_Port GPIOB
#define U3_DE_Pin GPIO_PIN_14
#define U3_DE_GPIO_Port GPIOB
#define P_OUT3_Pin GPIO_PIN_15
#define P_OUT3_GPIO_Port GPIOB
#define P_OUT2_Pin GPIO_PIN_8
#define P_OUT2_GPIO_Port GPIOA
#define P_OUT1_Pin GPIO_PIN_9
#define P_OUT1_GPIO_Port GPIOA
#define U2_RXD_Pin GPIO_PIN_15
#define U2_RXD_GPIO_Port GPIOA
#define U2_TXD_Pin GPIO_PIN_3
#define U2_TXD_GPIO_Port GPIOB
#define P_OUT5_Pin GPIO_PIN_5
#define P_OUT5_GPIO_Port GPIOB
#define U1_TXD_Pin GPIO_PIN_6
#define U1_TXD_GPIO_Port GPIOB
#define U1_RXD_Pin GPIO_PIN_7
#define U1_RXD_GPIO_Port GPIOB
#define CAN_RXD_Pin GPIO_PIN_8
#define CAN_RXD_GPIO_Port GPIOB
#define CAN_TXD_Pin GPIO_PIN_9
#define CAN_TXD_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
