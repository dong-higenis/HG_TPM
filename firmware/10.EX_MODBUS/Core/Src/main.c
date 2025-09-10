/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body - Modbus RTU to FDCAN Bridge
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
 * Modbus RTU 를 사용하는 풍량센서 측정값을 실시간으로 받아보는 예제입니다.
 */

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* Modbus RTU 프로토콜 정의 */
#define MODBUS_SLAVE_ADDR 0x01     // 풍량센서 기본 주소
#define MODBUS_FUNC_READ_HOLD 0x03 // Modbus 기능코드 (0x03은 레지스터 읽기)
/* 풍량센서 레지스터 주소 */
#define WIND_SPEED_REG 0x0000 // 풍속 레지스터 (데이터 시트 참조)
// 위 세 값을 기반으로 센서에게 데이터 요청!!

/* 프레임 크기 정의 */
#define MODBUS_REQUEST_SIZE 8   // 모드버스 요청 프레임 크기
#define MODBUS_RESPONSE_MAX 256 // 모드버스 응답 최대 크기

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* Modbus RTU 요청 프레임 구조체 */
typedef struct
{
    uint8_t slave_addr;                     // 슬레이브 주소
    uint8_t function_code;                  // 기능 코드
    uint16_t start_addr;                    // 시작 레지스터 주소 (빅엔디안)
    uint16_t register_count;                // 레지스터 개수 (빅엔디안) ( 풍속값만 받아올것이므로 사실상 1 )
    uint16_t crc;                           // CRC16 (리틀엔디안)
} __attribute__((packed)) Modbus_Request_t; // 패딩 줄이기

/* Modbus RTU 응답 프레임 구조체 */
typedef struct
{
    uint8_t slave_addr;    // 슬레이브 주소
    uint8_t function_code; // 기능 코드
    uint8_t byte_count;    // 데이터 바이트 수
    uint8_t data[64];      // 레지스터 데이터
    uint16_t crc;          // CRC16 (리틀엔디안)
} __attribute__((packed)) Modbus_Response_t;

// 센서 데이터 저장
typedef struct
{
    float wind_speed;   // 풍속 (m/s)
    uint32_t timestamp; // 타임스탬프
    uint8_t valid;      // 데이터 유효성
} Wind_Sensor_Data_t;   // 센서에서 읽어온 데이터를 저장하는 구조체입니다.

Wind_Sensor_Data_t sensor_data = {0}; // 센서 데이터를 저장

// Modbus RTU 관련 변수
uint8_t modbus_rx_buffer[MODBUS_RESPONSE_MAX]; // UART로 받은 Modbus 응답을 저장하는 버퍼
static uint16_t modbus_rx_index = 0;           // 현재 버퍼에 저장된 바이트 수
static uint8_t modbus_frame_ready = 0;         // 완전한 프레임을 받았는지 표시
static uint8_t rx_byte;                        // UART 인터럽트로 받은 1바이트

volatile uint32_t last_rx_time = 0; // 마지막 수신 시간 (타임아웃 체크용)

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
HAL_StatusTypeDef modbusRequest(uint8_t slave_addr, uint8_t func_code, uint16_t start_addr, uint16_t reg_count);
HAL_StatusTypeDef modbusResponse(uint8_t *response_data, uint16_t length);
static uint16_t calcCRC16(uint8_t *data, uint16_t length);
void requestSensorData(void);
void parseSensorData(Modbus_Response_t *response);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// printf 함수 리디렉션
int __io_putchar(int ch)
{
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 100);
    return ch;
}

/**
 * @brief Modbus RTU 관련
 */

// Modbus RTU CRC16 계산 함수
static uint16_t calcCRC16(uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;                // 1111 1111 1111 1111
    for (uint16_t i = 0; i < length; i++) // 들어온 모든 데이터에 대해..
    {
        crc ^= data[i];                       // 0xFFFF 와 XOR 연산
        for (uint8_t bit = 0; bit < 8; bit++) // 모든 비트에 대해 ..
        {
            if (crc & 0x0001) // LSB가 1이라면..
            {
                crc = (crc >> 1) ^ 0xA001; // 오른쪽 쉬프트 한칸 -> 0xA001과 XOR연산
            }
            else
            {
                crc = crc >> 1; // LSB가 0이라면... 쉬프트만...
            }
        }
    }
    return crc;
}

// Modbus RTU 요청 전송
HAL_StatusTypeDef modbusRequest(uint8_t slave_addr, uint8_t func_code, uint16_t start_addr, uint16_t reg_count)
{
    uint8_t request_frame[MODBUS_REQUEST_SIZE];
    uint16_t crc;

    // 프레임 구성
    request_frame[0] = slave_addr;
    request_frame[1] = func_code;
    request_frame[2] = (start_addr >> 8) & 0xFF; // 시작 주소 상위 바이트
    request_frame[3] = start_addr & 0xFF;        // 시작 주소 하위 바이트
    request_frame[4] = (reg_count >> 8) & 0xFF;  // 레지스터 개수 상위 바이트
    request_frame[5] = reg_count & 0xFF;         // 레지스터 개수 하위 바이트

    // CRC 계산 (첫 6바이트에 대해)
    crc = calcCRC16(request_frame, 6);

    request_frame[6] = crc & 0xFF;        // CRC 하위 바이트 (리틀엔디안)
    request_frame[7] = (crc >> 8) & 0xFF; // CRC 상위 바이트

/**
 * @brief UART3(Modbus)로 데이터 송신
 *  - 반이중 통신(모드버스 RTU) 특성상 송신 시 DE 핀을 HIGH로 (송신모드) 설정합니다.
 *  - 모든 데이터가 전송 완료(TC 플래그 set)될 때까지 대기하여, 마지막 바이트까지 온전히 라인에 출력됨을 보장합니다.
 *  - TC 확인 후 DE 핀을 LOW로 변경해 수신모드로 전환합니다.
 *  - (이 과정이 없으면 마지막 바이트 송신 전에 수신모드로 바뀌어 모드버스 통신 오류가 발생할 수 있음)
 */
    // UART3(RS485)로 전송
    HAL_GPIO_WritePin(Modbus_DE_GPIO_Port, Modbus_DE_Pin, GPIO_PIN_SET);
    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart3, request_frame, MODBUS_REQUEST_SIZE, 1000);
    while (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_TC) == RESET); // 전송 완료 기다림
    HAL_GPIO_WritePin(Modbus_DE_GPIO_Port, Modbus_DE_Pin, GPIO_PIN_RESET);

    if (status != HAL_OK)
    {
        printf("Modbus Request Failed: Status=%d\r\n", status);
    }
    return status;
}

// Modbus RTU 응답 처리
HAL_StatusTypeDef modbusResponse(uint8_t *response_data, uint16_t length)
{
    // CRC 계산
    uint16_t calc_crc = calcCRC16(response_data, length - 2);

    // Modbus RTU는 리틀엔디안 표준 사용
    uint16_t recv_crc = response_data[length - 2] | (response_data[length - 1] << 8);

    if (calc_crc != recv_crc)
    {
        printf("[ERROR] CRC mismatch: calc=0x%04X, recv=0x%04X\r\n", calc_crc, recv_crc);
        return HAL_ERROR;
    }

    // 슬레이브 주소 확인
    Modbus_Response_t *response = (Modbus_Response_t *)response_data;
    if (response->slave_addr != MODBUS_SLAVE_ADDR)
    {
        return HAL_ERROR;
    }

    parseSensorData(response);
    return HAL_OK;
}

// 풍량센서 응답 데이터 파싱
void parseSensorData(Modbus_Response_t *response)
{
    if (response->function_code == MODBUS_FUNC_READ_HOLD && response->byte_count >= 2)
    {
        // 풍속 데이터 (첫 번째 레지스터)
        uint16_t wind_raw = (response->data[0] << 8) | response->data[1];
        sensor_data.wind_speed = wind_raw * 0.1f; // 0.1 m/s 단위
        sensor_data.timestamp = HAL_GetTick();
        sensor_data.valid = 1;
        printf("Wind Speed: %.1f m/s\r\n", sensor_data.wind_speed);
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
  MX_USART3_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

    // Modbus RTU 수신 시작
    HAL_UART_Receive_IT(&huart3, &rx_byte, 1);

    printf("Modbus RTU Bridge Started!\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

    uint32_t last_request_time = 0;

    while (1)
    {

        // 100ms마다 센서 데이터 요청
        if (HAL_GetTick() - last_request_time >= 100)
        {
            // 이전 데이터 정리
            modbus_rx_index = 0;
            modbus_frame_ready = 0;

            // 센서 데이터 요청
            modbusRequest(MODBUS_SLAVE_ADDR, MODBUS_FUNC_READ_HOLD, WIND_SPEED_REG, 2);

            // 타이밍 리셋
            last_request_time = HAL_GetTick();
            last_rx_time = HAL_GetTick();
        }

        // 5ms 동안 새로운 바이트가 오지 않으면 프레임 완료로 판단
        if ((modbus_rx_index > 0) && (modbus_frame_ready == 0) &&
            ((HAL_GetTick() - last_rx_time) > 5))
        {
            modbus_frame_ready = 1; // 프레임 완료 플래그 설정
        }

        // Modbus 응답 처리
        if (modbus_frame_ready)
        {

            // 최소 길이 체크 (5바이트 이상, CRC를 제외한 나머지 프레임 구성요소가 5바이트)
            if (modbus_rx_index >= 5)
            {
                modbusResponse(modbus_rx_buffer, modbus_rx_index);
            }
            else
            {
                printf("[ERROR] Frame too short: %d bytes\r\n", modbus_rx_index);
            }
            // 버퍼 리셋
            modbus_rx_index = 0;
            modbus_frame_ready = 0;
            memset(modbus_rx_buffer, 0, sizeof(modbus_rx_buffer));
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

// UART 수신 콜백 ( UART3 == 센서 )
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        // 수신된 바이트 저장
        modbus_rx_buffer[modbus_rx_index++] = rx_byte;
        last_rx_time = HAL_GetTick();

        if (modbus_rx_index >= sizeof(modbus_rx_buffer) - 1)
        {
            printf("\r\n[DEBUG] Buffer full, frame ready\r\n");
        }
        // 다음 바이트 수신 준비
        HAL_UART_Receive_IT(&huart3, &rx_byte, 1);
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
