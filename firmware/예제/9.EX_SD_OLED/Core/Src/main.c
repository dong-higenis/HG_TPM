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
 * 초보자를 위한 요약
 * ------------------
 * - SD 카드가 꽂히면 자동으로 마운트, 빠지면 언마운트합니다.
 * - UART 터미널(TeraTerm 등)에서 명령을 입력해 파일 열기/쓰기/읽기/용량확인과
 *   OLED 이미지 표시를 테스트할 수 있습니다.
 *
 * 사용 가능한 명령
 * ----------------
 * help
 * mount / unmount
 * open <파일명> / close
 * write <텍스트>
 * read <파일명>
 * size
 * image <bmp파일명>     // BMP를 SD에서 읽어 OLED에 표시
 * antiflicker         // (라이브러리 제공 시) OLED 깜빡임 저감 모드
 * camera              // (라이브러리 제공 시) 카메라 최적 모드
 *
 * 주의(실전)
 * ----------
 * - 상용 제품에서는 예외 처리, 전원/탈착 상황 보호 로직, 멀티스레드 보호(뮤텍스) 등이 필요합니다.
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
#include "fatfs_sd.h"
#include "oled.h"
#include "font.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define OLED_WIDTH   256
#define OLED_HEIGHT   64
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
/* FatFs 핸들 및 상태 */
FATFS   fs;
FATFS  *pfs;
FIL     fil;
FRESULT fres;
DWORD   fre_clust;

/* 간단한 텍스트 버퍼 (한 줄 입출력용) */
static char buffer[100];

/* SD 카드 삽입 상태(엣지 검출용) */
static uint8_t previousCardState = 0;

/* 파일 열림 상태 (0=열림, 1=닫힘). 기본 닫힘 */
int8_t closeFlag = 1;

/* 파일 I/O 길이 */
UINT bw, br;

/* UART 라인 입력 처리용 */
static char     rxBuffer[100];   /* 현재 라인 버퍼 */
static uint8_t  rxIndex = 0;     /* 버퍼 내 위치 */
static uint8_t  commandReady = 0;/* 1줄(엔터) 완료 플래그 */
static volatile uint8_t g_rx;    /* 인터럽트로 받는 1바이트 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* SD 카드 및 파일 I/F */
uint8_t SD_IsCardDetected(void);
void    SDMount(void);
void    SDUnmount(void);
void    OpenFile(char* fileName);
void    CloseFile(void);
void    CheckSize(void);
void    WriteFile(char* text);
void    ReadFile(char* fileName);

/* 명령 파서 */
void    ProcessCommand(char *command);
void    ShowHelp(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* printf() → USART1 리다이렉트 */
int __io_putchar(int ch)
{
  (void)HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, 100);
  return ch;
}

/* SD 카드 감지 (CD 핀 LOW = 삽입) */
uint8_t SD_IsCardDetected(void)
{
  return (HAL_GPIO_ReadPin(SD_CD_GPIO_Port, SD_CD_Pin) == GPIO_PIN_RESET);
}

/* SD 카드 마운트 */
void SDMount(void)
{
  fres = f_mount(&fs, "", 0);
  if (fres == FR_OK)
  {
    printf("SD Card mounted successfully.\r\n");
    oled_drawString(30, 0, "                  ", &font_07x10, 15);
    oled_drawString(30, 0, "SD Card Mounted",   &font_07x10, 15);
  }
  else
  {
    printf("SD Card mount error! (FRESULT=%d)\r\n", fres);
  }
}

/* SD 카드 언마운트 */
void SDUnmount(void)
{
  /* 열린 파일이 있으면 먼저 닫기(안전) */
  if (closeFlag == 0) CloseFile();

  fres = f_mount(NULL, "", 0);
  if (fres == FR_OK)
  {
    printf("SD Card unmounted successfully.\r\n");
    oled_drawString(30, 0, "                  ", &font_07x10, 15);
    oled_drawString(30, 0, "SD Card Un-Mounted", &font_07x10, 15);
  }
  else
  {
    printf("SD Card unmount error! (FRESULT=%d)\r\n", fres);
  }
}

/* 파일 열기(없으면 생성). 항상 append 위치로 이동 */
void OpenFile(char* fileName)
{
  if (closeFlag == 0)
  {
    printf("A file is already open. Close it first.\r\n");
    return;
  }

  fres = f_open(&fil, fileName, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
  if (fres != FR_OK)
  {
    printf("Failed to open/create '%s' (FRESULT=%d)\r\n", fileName, fres);
    return;
  }

  fres = f_lseek(&fil, f_size(&fil)); /* append 보장 */
  if (fres != FR_OK)
  {
    printf("Seek end failed. (FRESULT=%d)\r\n", fres);
    f_close(&fil);
    return;
  }

  printf("File '%s' ready. size=%lu bytes\r\n", fileName, (unsigned long)f_size(&fil));
  oled_drawString(30, 20, "                           ", &font_07x10, 15);
  oled_drawString(30, 20, "File Ready!!",               &font_07x10, 15);
  closeFlag = 0;
}

/* 열린 파일 닫기 */
void CloseFile(void)
{
  fres = f_close(&fil);
  if (fres == FR_OK)
  {
    printf("File closed.\r\n");
    oled_drawString(30, 20, "                           ", &font_07x10, 15);
    oled_drawString(30, 20, "File Closed!!",             &font_07x10, 15);
  }
  else
  {
    printf("File close failed. (FRESULT=%d)\r\n", fres);
  }
  closeFlag = 1;
}

/* SD 용량 확인 (KB 단위) */
void CheckSize(void)
{
  fres = f_getfree("", &fre_clust, &pfs);
  if (fres == FR_OK)
  {
    /* 섹터는 512바이트 → KB 환산은 *0.5 (512/1024) */
    uint32_t freeKB  = (uint32_t)(fre_clust * pfs->csize * 0.5f);
    uint32_t totalKB = (uint32_t)((pfs->n_fatent - 2) * pfs->csize * 0.5f);
    printf("Free: %lu KB / Total: %lu KB\r\n",
           (unsigned long)freeKB, (unsigned long)totalKB);
  }
  else
  {
    printf("Failed to get free space. (FRESULT=%d)\r\n", fres);
  }
}


/* 현재 열린 파일 끝에 한 줄을 append */
void WriteFile(char* text)
{
  if (closeFlag)
  {
    printf("No file is open. Use: open <filename>\r\n");
    return;
  }

  /* 항상 끝으로 이동(append 보장) */
  fres = f_lseek(&fil, f_size(&fil));
  if (fres != FR_OK) {
    printf("Seek end failed. (FRESULT=%d)\r\n", fres);
    return;
  }

  /* 안전하게 개행을 붙여 한 줄로 기록 */
  int len = snprintf(buffer, sizeof(buffer), "%s\r\n", text);
  if (len < 0)
  {
    printf("Format error.\r\n");
    return;
  }

  fres = f_write(&fil, buffer, (UINT)strlen(buffer), &bw);
  if (fres == FR_OK)
  {
    f_sync(&fil); /* 즉시 저장(전원 차단 대비) */
    printf("Write OK: %u bytes\r\n", (unsigned)bw);
  }
  else
  {
    printf("Write failed. (FRESULT=%d)\r\n", fres);
  }
}

/* 파일 읽기(주의: 데모용으로 최대 buffer 크기까지만 표시) */
void ReadFile(char* fileName)
{
  /* 다른 파일이 열려 있으면 닫고 읽기 전용으로 연다 */
  if (closeFlag == 0)
  {
	  CloseFile();
  }

  fres = f_open(&fil, fileName, FA_READ);
  if (fres != FR_OK)
  {
    printf("Failed to open '%s' for reading. (FRESULT=%d)\r\n", fileName, fres);
    return;
  }

  memset(buffer, 0, sizeof(buffer));
  fres = f_read(&fil, buffer, sizeof(buffer) - 1, &br);
  if (fres == FR_OK)
  {
    printf("----- FILE: %s -----\r\n", fileName);
    if (br > 0)
    {
      printf("%s", buffer);
      if (buffer[strlen(buffer) - 1] != '\n')
    	{
    	  printf("\r\n");
    	}
      printf("----- %u bytes shown (truncated if large) -----\r\n", (unsigned)br);
    }
    else
    {
      printf("(empty)\r\n");
    }
  }
  else
  {
    printf("Read failed. (FRESULT=%d)\r\n", fres);
  }

  f_close(&fil);
}

/* 한 줄 명령 해석 및 실행 */
void ProcessCommand(char *command)
{
  printf("Command received: %s\r\n", command);

  if (strcmp(command, "help") == 0)
  {
    ShowHelp();
  }
  else if (strcmp(command, "mount") == 0)
  {
    SDMount();
  }
  else if (strcmp(command, "unmount") == 0)
  {
    SDUnmount();
  }
  else if (strncmp(command, "open ", 5) == 0)
  {
    OpenFile(command + 5);
  }
  else if (strcmp(command, "close") == 0)
  {
    CloseFile();
  }
  else if (strncmp(command, "write ", 6) == 0)
  {
    WriteFile(command + 6);
  }
  else if (strncmp(command, "read ", 5) == 0)
  {
    ReadFile(command + 5);
  }
  else if (strncmp(command, "image ", 6) == 0)
  {
    DisplayImageFromSD_Enhanced(command + 6);
  }
  else if (strcmp(command, "size") == 0)
  {
    CheckSize();
  }
  else if (strcmp(command, "camera") == 0)
  {
    OLED_setCameraMode();
  }
  else {
    printf("Unknown command: %s\r\nType 'help' for available commands.\r\n", command);
  }
}

/* 명령 도움말 */
void ShowHelp(void)
{
  printf("\r\n=== Available Commands ===\r\n");
  printf("help               - Show this help\r\n");
  printf("mount              - Mount SD card\r\n");
  printf("unmount            - Unmount SD card\r\n");
  printf("open <filename>    - Open (create if not exist) & append mode\r\n");
  printf("close              - Close current file\r\n");
  printf("write <text>       - Append one line to current file\r\n");
  printf("read <filename>    - Read file content (up to buffer size)\r\n");
  printf("image <filename>   - Display 32bpp BMP on OLED\r\n");
  printf("size               - Show SD free/total space (KB)\r\n");
  printf("camera             - OLED camera mode (if available)\r\n");
  printf("==========================\r\n");
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  if (MX_FATFS_Init() != APP_OK) {
    Error_Handler();
  }
  MX_SPI3_Init();

  /* USER CODE BEGIN 2 */
  /* OLED 초기화(라이브러리 제공 가정) */
  OLED_init();
  OLED_fill(0); /* 화면 클리어 */

  /* UART 1바이트 인터럽트 수신 시작 */
  HAL_UART_Receive_IT(&huart1, (uint8_t*)&g_rx, 1);

  /* 시작 메시지 */
  printf("\r\n=== SD Card Control System ===\r\n");
  printf("Type 'help' for available commands.\r\n");
  printf("Ready> ");

  /* USER CODE END 2 */

  /* Infinite loop */
  while (1) {

    /* SD 카드 삽입/제거 감지(엣지) */
    uint8_t currentCardState = SD_IsCardDetected();
    if (currentCardState && !previousCardState)
    {
      HAL_Delay(200); /* 간단한 디바운싱 */
      SDMount();
    }
    else if (!currentCardState && previousCardState)
    {
      SDUnmount();
    }
    previousCardState = currentCardState;

    /* 한 줄 명령 처리 */
    if (commandReady)
    {
      ProcessCommand(rxBuffer);
      commandReady = 0;
      printf("Ready> ");
    }

    HAL_Delay(10);
  }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM       = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN       = 40;
  RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ       = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR       = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                   | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
 * @brief UART 수신 인터럽트 콜백
 *        - 1바이트씩 수신하고 엔터('\r' 또는 '\n')가 오면 한 줄 명령으로 처리
 *        - 에코백 추가(터미널 가독성↑)
 */
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
      // HAL_UART_Transmit(&huart1, (uint8_t*)&g_rx, 1, 10); // 이 줄 제거 또는 주석
    }
    HAL_UART_Receive_IT(&huart1, (uint8_t*)&g_rx, 1);
  }
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  __disable_irq();
  while (1) {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
  /* 예: printf("Wrong parameters value: file %s on line %lu\r\n", file, line); */
}
#endif /* USE_FULL_ASSERT */
