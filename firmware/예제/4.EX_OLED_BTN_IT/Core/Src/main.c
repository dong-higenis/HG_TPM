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


/*
 *
 * 버튼누름 여부를 인터럽트로 확인하는 방식의 예제입니다. (인터럽트 예제)
 *
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

// 버튼의 '현재 안정 상태' (1=눌림, 0=떼짐)
volatile uint8_t btn1_state = 0;
volatile uint8_t btn2_state = 0;

// OLED를 갱신하도록 트리거하는 플래그
volatile uint8_t btn1_oled_flag = 0;
volatile uint8_t btn2_oled_flag = 0;

// 인터럽트에서 기록하는 'keep 플래그'와 마지막 엣지 시간
volatile uint8_t  btn1_keep   = 0, btn2_keep   = 0;
volatile uint32_t btn1_last_edge = 0, btn2_last_edge = 0;

// 기계식 버튼 튀는 현상을 흡수하기 위한 디바운스 시간(ms)
#define DEBOUNCE_MS 10  // 버튼이나 환경마다 다를수있으니 조정해보세요!
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief 인터럽트로부터 일정 시간(DEBOUNCE_MS) 경과 후
 *        실제 핀 레벨을 읽어 상태를 '확정'하는 함수
 *        - 인터럽트 콜백에서는 '언제 변화가 있었는지'만 기록
 *        - 여기서 그 변화가 안정화되었는지 확인하고 반영
 */



static void DebouncedBTN(void)
{
  uint32_t now = HAL_GetTick();

  // BTN1: pending이면 DEBOUNCE_MS 지난 뒤 현재 레벨을 읽어 상태 확정, (한번더 체크)
  if (btn1_keep && (now - btn1_last_edge) >= DEBOUNCE_MS)
  {
    btn1_keep = 0; // 이번 변화에 대한 판정을 시작하므로 keep 클리어


    uint8_t cur = (HAL_GPIO_ReadPin(BTN1_GPIO_Port, BTN1_Pin) == GPIO_PIN_RESET); // 현재 핀상태

    // 상태가 바뀌었을 때만 OLED/LED를 갱신하도록 플래그 및 출력 처리
    if (cur != btn1_state)
    {
      btn1_state = cur;
      btn1_oled_flag = 1; // 메인 루프에서 한 번만 OLED 갱신

      // 버튼 눌림에 따라 LED on/off (active-low LED 가정)
      HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin,
                        btn1_state ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
  }

  // BTN2: BTN1과 동일한 로직
  if (btn2_keep && (now - btn2_last_edge) >= DEBOUNCE_MS)
  {
    btn2_keep = 0;

    uint8_t cur = (HAL_GPIO_ReadPin(BTN2_GPIO_Port, BTN2_Pin) == GPIO_PIN_RESET);

    if (cur != btn2_state)
    {
      btn2_state = cur;
      btn2_oled_flag = 1;

      HAL_GPIO_WritePin(LINK_LED_GPIO_Port, LINK_LED_Pin,
                        btn2_state ? GPIO_PIN_RESET : GPIO_PIN_SET);
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
	OLED_init(); // oled 초기화

	OLED_fill(0); // oled 전체를 검은색으로 칠함

	// (x좌표, y좌표, String, font, 밝기)
	oled_drawString(20, 0, "Button List", &font_07x10, 15); // 화면 위쪽
	oled_drawString(0, 20, "Button1", &font_07x10, 15); // 화면 왼쪽
	oled_drawString(60, 20, "OFF", &font_07x10, 1); // 화면 중앙, OFF는 어둡게...
	oled_drawString(0, 40, "Button2", &font_07x10, 15); // Button1 아래
	oled_drawString(60, 40, "OFF", &font_07x10, 1);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
		DebouncedBTN();
		  if (btn1_oled_flag || btn2_oled_flag)
		  {
		    btn1_oled_flag = 0;
		    btn2_oled_flag = 0;

		    if (btn1_state && btn2_state)
		    {
		      oled_drawString(60, 20, "ON ",  &font_07x10, 15);
		      oled_drawString(60, 40, "ON ",  &font_07x10, 15);

		    } else if (btn1_state && !btn2_state)
		    {
		      oled_drawString(60, 20, "ON ",  &font_07x10, 15);
		      oled_drawString(60, 40, "OFF",  &font_07x10, 1);

		    } else if (!btn1_state && btn2_state)
		    {
		      oled_drawString(60, 20, "OFF",  &font_07x10, 1);
		      oled_drawString(60, 40, "ON ",  &font_07x10, 15);

		    } else
		    {
		      oled_drawString(60, 20, "OFF",  &font_07x10, 1);
		      oled_drawString(60, 40, "OFF",  &font_07x10, 1);

		    }
		  }


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
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
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
//=======================================================





//========= 인터럽트 발생 콜백함수 추가!! ======================


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  uint32_t now = HAL_GetTick();
  if (GPIO_Pin == BTN1_Pin)
  {
    btn1_last_edge = now;
    btn1_keep = 1;          // 상태 확정은 메인루프로 미룸
  } else if (GPIO_Pin == BTN2_Pin)
  {
    btn2_last_edge = now;
    btn2_keep = 1;
  }
}


//========= 인터럽트 발생 콜백함수 추가!! ======================




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
	while (1) {
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
