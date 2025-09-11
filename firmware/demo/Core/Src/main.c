/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : SD 카드 마운트 & 파일 입출력 (SPI + FatFs) + OLED 표시 데모
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

/*
 *
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/

#include "main.h"
#include "dma.h"
#include "app_fatfs.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SD/sd_func.h"
#include "User/user_func.h"
#include "ap/ap_sensor.h"
#include "oled_eyes.h"
#include "expression.h"

#include "modbus_def.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

static wind_sensor_t wind_sensor;

/* USER CODE BEGIN PV */

// UART 명령 파서용
char rxBuffer[100];       // 입력 라인 버퍼
uint8_t rxIndex = 0;      // 현재 입력 위치
uint8_t commandReady = 0; // 한 줄(엔터) 입력 완료 플래그
volatile uint8_t g_rx;    // UART 수신 1바이트(인터럽트로 채움)

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* SD 카드 및 파일 I/F */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* printf() → USART1 리다이렉트 */
int __io_putchar(int ch)
{
  (void)HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 100);
  return ch;
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  if (MX_FATFS_Init() != APP_OK)
  {
    Error_Handler();
  }
  MX_SPI3_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

  HAL_UART_Receive_IT(&huart1, (uint8_t *)&g_rx, 1);

  sdCardInit();
  if (apOledInit())
  {
    apOledUpdate();

    // apOledTest();
    eyesInit();
    // 시작 애니메이션
    eyesSleep();
    eyesWakeup();
  }
  apSensorInit(&wind_sensor);
  uint32_t animation_timer = 0;
  uint32_t blink_timer = 0;
  int current_animation = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    apSensorRequest(&wind_sensor);
    apSensorUpdate(&wind_sensor);

    static uint32_t no_sensor_timer = 0;
    uint32_t current_time = HAL_GetTick();

    if (apSensorIsValid(&wind_sensor))
    {
      float speed = apSensorGetSpeed(&wind_sensor);
      eyesInstantExpression((double)speed);
      no_sensor_timer = current_time; // 센서 유효하면 타이머 리셋
    }
    else
    {
      // 센서 무효 시 2초마다 기본 깜빡임
      if ((current_time - no_sensor_timer) > 2000)
      {
        printf("No sensor - default blink\r\n");
        eyesBlink(12);
        no_sensor_timer = current_time;
      }
    }
    sdIsCardDetected();
    if (commandReady)
    {
      userInput(rxBuffer);
      commandReady = 0;
      printf("Ready> ");
    }

    HAL_Delay(10);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    if (g_rx == '\r' || g_rx == '\n')
    {
      rxBuffer[rxIndex] = '\0';
      commandReady = 1;
      rxIndex = 0;
      printf("\r\n");
    }
    else if (rxIndex < sizeof(rxBuffer) - 1)
    {
      rxBuffer[rxIndex++] = g_rx;
    }
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&g_rx, 1);
  }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART3)
  {
    // wind_sensor 구조체에 수신 데이터 설정
    wind_sensor.rx_index = Size;
    wind_sensor.frame_ready = true;

    // 다음 수신을 위해 DMA 재시작
    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, wind_sensor.rx_buffer,
                                 sizeof(wind_sensor.rx_buffer));
  }
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
