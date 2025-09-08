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
 *  SPI신호로 SD카드를 마운트 시키고, 파일 입출력을 하는 예제입니다.
 * 주의
 * ----
 * - 실제 프로젝트에서는 오류/예외 처리와 뮤텍스(멀티스레드)등의 고려가 더 필요합니다.
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
#include "fatfs_sd.h"
#include "string.h"

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

// FatFs 관련 핸들
FATFS fs;
FATFS *pfs;
FIL fil;
FRESULT fres;
DWORD fre_clust;

// 간단한 버퍼들
char buffer[100];

// SD 카드 삽입 상태 기억용
static uint8_t previousCardState = 0;

// 파일 열림 상태 (0: 열림, 1: 닫힘) — 기본은 닫힘
int8_t closeFlag = 1;

// 파일 I/O 길이
uint32_t bw, br;

// UART 명령 파서용
char rxBuffer[100];          // 입력 라인 버퍼
uint8_t rxIndex = 0;         // 현재 입력 위치
uint8_t commandReady = 0;    // 한 줄(엔터) 입력 완료 플래그
volatile uint8_t g_rx;       // UART 수신 1바이트(인터럽트로 채움)


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* 컴파일 경고 없애기용 프로토타입을 선언 */
uint8_t SD_IsCardDetected(void);
void SDMount(void);
void SDUnmount(void);
void OpenFile(char* fileName);
void CloseFile(void);
void CheckSize(void);
void WriteFile(char* text);
void ReadFile(char* fileName);
void ProcessCommand(char *command);
void ShowHelp(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int __io_putchar(int ch) {
	(void) HAL_UART_Transmit(&huart1, (uint8_t*) &ch, 1, 100);
	return ch;
}

/* SD카드 감지 함수 */
uint8_t SD_IsCardDetected(void) {
	// CD 핀이 LOW면 카드 삽입됨
	return (HAL_GPIO_ReadPin(SD_CD_GPIO_Port, SD_CD_Pin) == GPIO_PIN_RESET);
}

/* SD카드 마운트 함수 */
void SDMount(void) {
	fres = f_mount(&fs, "", 0);
	if (fres == FR_OK) {
		printf("SD Card mounted Successfully!\r\n");
	} else if (fres != FR_OK) {
		printf("SD Card mount error!!\r\n");
	}
}

/* SD 카드 언마운트 (열린 파일이 있으면 먼저 닫기) */
void SDUnmount(void) {

	if (closeFlag == 0) // 열린 파일 있으면 닫기
	{
	    CloseFile();
	}

	fres = f_mount(NULL, "", 0);
	if (fres == FR_OK) {
		printf("SD Card Un-mounted Successfully!\r\n");
	} else if (fres != FR_OK) {
		printf("SD Card Un-mount error!!\r\n");
	}
}

/* 파일 열기(없으면 생성). append 모드로 사용 */
void OpenFile(char* fileName)
{
    if(closeFlag == 0)
    {
        printf("File already open! Close it first.\r\n");
        return;
    }

    // 파일이 있으면 열고, 없으면 생성하여 append 모드로 사용
    fres = f_open(&fil, fileName, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);

    if(fres == FR_OK)
    {
    	// 항상 파일 끝으로 이동하여 이어쓰기(Append)
        f_lseek(&fil, f_size(&fil));

        printf("File '%s' ready for writing!\r\n", fileName);
        if(f_size(&fil) > 0)
        {
            printf("File size: %lu bytes\r\n", f_size(&fil));
        }
        else
        {
            printf("New file created.\r\n");
        }
    }
    else
    {
        printf("Failed to open/create file '%s'. Error: %d\r\n", fileName, fres);
        return;
    }

    closeFlag = 0; // 이제 열림 상태
}

/* 열린 파일 닫기 */
void CloseFile(void) {
	fres = f_close(&fil);
	if (fres == FR_OK) {
		printf("File Closed !\r\n");
	} else if (fres != FR_OK) {
		printf("File Close Failed... \r\n");
	}
	closeFlag = 1;
}

/* SD 카드 여유 공간 조회 (KB 단위로 표시) */
void CheckSize(void) {
  fres = f_getfree("", &fre_clust, &pfs);
  if (fres == FR_OK) {
    // 클러스터 수 * 클러스터당 섹터수 * 섹터당 512바이트 → KB로 환산( /1024 = *0.5)
    uint32_t freeKB = (uint32_t)(fre_clust * pfs->csize * 0.5f);
    uint32_t totalKB = (uint32_t)((pfs->n_fatent - 2) * pfs->csize * 0.5f);
    printf("Free: %lu KB / Total: %lu KB\r\n", freeKB, totalKB);
  } else {
    printf("Failed to get free space. (FRESULT=%d)\r\n", fres);
  }
}

/* 현재 열린 파일 끝에 한 줄을 추가로 기록 */
void WriteFile(char* text)
{
    if (closeFlag)
    {
        printf("No file is open! Use 'open <filename>' first.\r\n");
        return;
    }

    // 안전하게 개행을 붙여 한 줄 단위로 기록
    snprintf(buffer, sizeof(buffer), "%s\r\n", text);

    // 파일 끝으로 이동 (append)
    fres = f_lseek(&fil, f_size(&fil));
    if(fres != FR_OK)
    {
        printf("Can't move to end of file\r\n");
        return;
    }

    sprintf(buffer, "%s\r\n", text);
    fres = f_write(&fil, buffer, strlen(buffer), &bw);

    if(fres == FR_OK)
    {
        printf("Writing Complete! %lu bytes written.\r\n", bw);
        f_sync(&fil);  // 즉시 저장(전원 차단에 대비)
    }
    else
    {
        printf("Writing Failed\r\n");
    }
}

/* 기존의 파일 내용 읽기 */
void ReadFile(char* fileName)
{
    // 현재 열린 파일이 있으면 닫기
    if (closeFlag == 0)
        CloseFile();

    fres = f_open(&fil, fileName, FA_READ);
    if (fres == FR_OK)
    {
        printf("File '%s' opened for reading.\r\n", fileName);
    }
    else
    {
        printf("Failed to open file '%s' for reading!\r\n", fileName);
        return;
    }

    // 파일 전체 읽기
    memset(buffer, 0, sizeof(buffer));
    fres = f_read(&fil, buffer, sizeof(buffer)-1, &br);

    if (fres == FR_OK && br > 0)
    {
        printf("-----------FILE CONTENT----------\r\n");
        printf("%s", buffer);
        if (buffer[strlen(buffer)-1] != '\n')
            printf("\r\n");
        printf("-----------END OF FILE-----------\r\n");
        printf("%lu bytes read.\r\n", br);
    }
    else
    {
        printf("File is empty or read failed!\r\n");
    }

    f_close(&fil);  // 읽기 후 파일 닫기
}

// ========== 인터페이스 함수 =================

/* 문자열 한 줄을 명령으로 해석해 실행 */
void ProcessCommand(char *command)
{
	printf("Command received: %s\r\n", command);

	if (strcmp(command, "mount") == 0)
	{
		SDMount();
	}
	else if (strcmp(command, "unmount") == 0)
	{
		SDUnmount();
	}
	else if (strncmp(command, "open ", 5) == 0)
	{
		char *filename = command + 5;
		OpenFile(filename);
	}
	else if (strcmp(command, "close") == 0)
	{
		CloseFile();
	}
	else if (strncmp(command, "write ", 6) == 0)
	{
		char *text = command + 6;
		WriteFile(text);
	}
	else if (strncmp(command, "read ", 5) == 0)
	{
		char *filename = command + 5;
		ReadFile(filename);
	}
	else if (strcmp(command, "size") == 0)
	{
		CheckSize();
	}
	else if (strcmp(command, "help") == 0)
	{
		ShowHelp();
	}
	else
	{
		printf("Unknown command: %s\r\n", command);
		printf("Type 'help' for available commands.\r\n");
	}
}

/* 지원 명령 리스트 출력 */
void ShowHelp(void)
{
	printf("\r\n=== Available Commands ===\r\n");
	printf("mount              - Mount SD card\r\n");
	printf("unmount            - Unmount SD card\r\n");
	printf("open <filename>    - Open file\r\n");
	printf("close              - Close current file\r\n");
	printf("write <text>       - Write text to file\r\n");
	printf("read <filename>    - Read file content\r\n");
	printf("size               - Check SD card free space\r\n");
	printf("help               - Show this help\r\n");
	printf("==========================\r\n");
}


/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

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
	if (MX_FATFS_Init() != APP_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN 2 */

	HAL_UART_Receive_IT(&huart1, &g_rx, 1); // UART 전역변수로 받기!

	// 시작 메시지
	printf("\r\n=== SD Card Control System ===\r\n");
	printf("Type 'help' for available commands.\r\n");
	printf("Ready> ");

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {

		uint8_t currentCardState = SD_IsCardDetected(); // 카드 삽입 감지

		if (currentCardState && !previousCardState) // 디바운싱
		{
			HAL_Delay(200); // 디바운싱
			SDMount();
		}
		else if (!currentCardState && previousCardState) // 카드 제거 감지
		{
			SDUnmount();
		}

		previousCardState = currentCardState;

		// 명령어 처리
		if (commandReady)
		{
			ProcessCommand(rxBuffer);
			commandReady = 0;
			printf("Ready> "); // 계속 사용자 입력을 감시
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
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

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
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

/**
 * @brief UART 수신 인터럽트 콜백
 *        1바이트씩 수신해 '\r' 또는 '\n'이 오면 한 줄 명령으로 처리한다.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    if (g_rx == '\r' || g_rx == '\n') {
      // 한 줄 종료 → 명령 처리 플래그 셋
      rxBuffer[rxIndex] = '\0';
      commandReady = 1;
      rxIndex = 0;
      printf("\r\n");  // 깔끔히 줄바꿈
    } else if (rxIndex < sizeof(rxBuffer) - 1) {
      // 일반 문자 → 버퍼에 축적, 동시에 에코백(선택)
      rxBuffer[rxIndex++] = g_rx;
      HAL_UART_Transmit(&huart1, (uint8_t*)&g_rx, 1, 10);
    }
    // 다음 1바이트 수신 예약
    HAL_UART_Receive_IT(&huart1, (uint8_t*)&g_rx, 1);
  }
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
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
