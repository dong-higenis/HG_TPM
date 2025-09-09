/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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

/**
 * @brief 버튼을 누르면 인터럽트 콜백함수가 호출됩니다.
 * - 해당 콜백함수 내에서는 감지변화만 체크하며,
 * - 메인 제어는 폴링방식으로 하고있는 예제입니다.
 */

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "font.h"
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

/* USER CODE BEGIN PV */

// 글씨 밝기
enum bright
{
  BRIGHT_OFF = 0, // 완전히 꺼짐 (보이지 않음)
  BRIGHT_LOW = 1, // 어둡게 (OFF 상태 표시용)
  BRIGHT_ON = 15  // 밝게 (ON 상태 표시용)
};

// 버튼 상태와 업데이트 플래그
volatile uint8_t btn1_keep = 0;
volatile uint8_t btn2_keep = 0;
volatile uint32_t btn1_last_edge = 0;
volatile uint32_t btn2_last_edge = 0;

uint8_t btn1_state = 0; // 현재 안정된 상태
uint8_t btn2_state = 0;
volatile uint8_t oled_update_needed = 0;

#define DEBOUNCE_MS 50

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void debouncedBTN(void)
{
  uint32_t now = HAL_GetTick();

  if (btn1_keep && (now - btn1_last_edge) >= DEBOUNCE_MS)
  {
    btn1_keep = 0;
    uint8_t cur = (HAL_GPIO_ReadPin(BTN1_GPIO_Port, BTN1_Pin) == GPIO_PIN_RESET);

    if (cur != btn1_state)
    {
      btn1_state = cur;
      oled_update_needed = 1;
    }
  }

  if (btn2_keep && (now - btn2_last_edge) >= DEBOUNCE_MS)
  {
    btn2_keep = 0;
    uint8_t cur = (HAL_GPIO_ReadPin(BTN2_GPIO_Port, BTN2_Pin) == GPIO_PIN_RESET);

    if (cur != btn2_state)
    {
      btn2_state = cur;
      oled_update_needed = 1;
    }
  }
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
  MX_SPI3_Init();
  /* USER CODE BEGIN 2 */
  // 가장 작은 폰트로 첫 글자만
  OLED_init();  // oled 초기화
  OLED_fill(0); // oled 전체를 검은색으로 칠함

  // (x좌표, y좌표, String, font, 밝기)
  OLED_drawString(20, 0, "Button List", &font_07x10, 15); // 화면 위쪽
  OLED_drawString(0, 20, "Button1", &font_07x10, 15);     // 화면 왼쪽
  OLED_drawString(60, 20, "OFF", &font_07x10, 1);         // 화면 중앙, OFF는 어둡게...
  OLED_drawString(0, 40, "Button2", &font_07x10, 15);     // Button1 아래
  OLED_drawString(60, 40, "OFF", &font_07x10, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    debouncedBTN();
    // 인터럽트에서 플래그가 설정되면 OLED 업데이트
    if (oled_update_needed)
    {
      oled_update_needed = 0; // 플래그 클리어

      // OLED 업데이트
      OLED_drawString(60, 20, btn1_state ? "ON " : "OFF",
                      &font_07x10, btn1_state ? BRIGHT_ON : BRIGHT_LOW);
      OLED_drawString(60, 40, btn2_state ? "ON " : "OFF",
                      &font_07x10, btn2_state ? BRIGHT_ON : BRIGHT_LOW);
    }

    // 메인 루프에서는 다른 작업 가능
    HAL_Delay(10);
  }

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
}
/* USER CODE END 3 */

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

/**
 * @brief 콜백은 버튼의 변화만을 감지하는 역할입니다.
 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  uint32_t now = HAL_GetTick();

  if (GPIO_Pin == BTN1_Pin)
  {
    btn1_last_edge = now;
    btn1_keep = 1;
  }

  if (GPIO_Pin == BTN2_Pin)
  {
    btn2_last_edge = now;
    btn2_keep = 1;
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
