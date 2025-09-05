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
 *  SPI신호로 SD카드를 마운트 시키는 예제입니다.
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
#include "fatfs_sd.h"
#include "string.h"
#include "oled.h"
#include "font.h"
#include <stdlib.h>  // malloc, free 함수를 위해 추가
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

FATFS fs;
FATFS *pfs;
FIL fil;
FRESULT fres;
DWORD fre_clust;
uint32_t totalSpace, freeSpace;
char buffer[100];
static uint8_t previousCardState = 0;
int8_t closeFlag = 1;

uint32_t bw, br;
char str[100];    // WriteFile, ReadFile에서 사용
uint8_t closedFlag = 1;  // ReadFile에서 사용 (closeFlag -> closedFlag)
char rxBuffer[100];  // UART 입력 버퍼
uint8_t rxIndex = 0;
uint8_t commandReady = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

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
		oled_drawString(30, 0, "                  ", &font_07x10, 15); // 화면 위쪽
		oled_drawString(30, 0, "SD Card Mounted", &font_07x10, 15); // 화면 위쪽
	} else if (fres != FR_OK) {
		printf("SD Card mount error!!\r\n");
	}
}
/* SD카드 마운트 해제 함수 */
void SDUnmount(void) {
	fres = f_mount(NULL, "", 0);
	if (fres == FR_OK) {
		printf("SD Card Un-mounted Successfully!\r\n");
		oled_drawString(30, 0, "                  ", &font_07x10, 15); // 화면 위쪽
		oled_drawString(30, 0, "SD Card Un-Mounted", &font_07x10, 15); // 화면 위쪽
	} else if (fres != FR_OK) {
		printf("SD Card Un-mount error!!\r\n");
	}
}

void OpenFile(char *fileName) {
	if (closeFlag == 0) {
		printf("File already open! Close it first.\r\n");
		return;
	}

	// 파일이 있으면 열고, 없으면 생성하여 append 모드로 사용
	fres = f_open(&fil, fileName, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);

	if (fres == FR_OK) {
		// 파일 끝으로 이동 (기존 내용 보존하며 append)
		f_lseek(&fil, f_size(&fil));

		printf("File '%s' ready for writing!\r\n", fileName);
		//====================================================================================
		oled_drawString(30, 20, "                           ", &font_07x10, 15); // 화면 위쪽
		oled_drawString(30, 20, "File Ready!!", &font_07x10, 15); // 화면 위쪽
		//====================================================================================
		if (f_size(&fil) > 0) {
			printf("File size: %lu bytes\r\n", f_size(&fil));
		} else {
			printf("New file created.\r\n");
		}
	} else {
		printf("Failed to open/create file '%s'. Error: %d\r\n", fileName,
				fres);
		return;
	}

	closeFlag = 0;
}

void CloseFile(void) {
	fres = f_close(&fil);
	if (fres == FR_OK) {
		printf("File Closed !\r\n");
		//====================================================================================
		oled_drawString(30, 20, "                           ", &font_07x10, 15); // 화면 위쪽
		oled_drawString(30, 20, "File Closed!!", &font_07x10, 15); // 화면 위쪽
		//====================================================================================
	} else if (fres != FR_OK) {
		printf("File Close Failed... \r\n");
	}
	closeFlag = 1;
}

void CheckSize(void) {

	fres = f_getfree("", &fre_clust, &pfs);
	totalSpace = (uint32_t) ((pfs->n_fatent - 2) * pfs->csize * 0.5);
	freeSpace = (uint32_t) ((fre_clust * pfs->csize * 0.5));
	char mSize[100];
	sprintf(mSize, "%lu", freeSpace);
	if (fres == FR_OK) {
		printf("The free Space is : ");
		printf(mSize);
		printf("\r\n");
	} else if (fres != FR_OK) {
		printf("The free Space could not be determined!\r\n");
	}
}

void WriteFile(char *text) {
	if (closeFlag) {
		printf("No file is open! Use 'open <filename>' first.\r\n");
		return;
	}

	// 파일 끝으로 이동 (append)
	fres = f_lseek(&fil, f_size(&fil));
	if (fres != FR_OK) {
		printf("Can't move to end of file\r\n");
		return;
	}

	sprintf(buffer, "%s\r\n", text);
	fres = f_write(&fil, buffer, strlen(buffer), &bw);

	if (fres == FR_OK) {
		printf("Writing Complete! %lu bytes written.\r\n", bw);
		f_sync(&fil);  // 즉시 저장
	} else {
		printf("Writing Failed\r\n");
	}
}

void ReadFile(char *fileName) {
	// 현재 열린 파일이 있으면 닫기
	if (closeFlag == 0)
		CloseFile();

	fres = f_open(&fil, fileName, FA_READ);
	if (fres == FR_OK) {
		printf("File '%s' opened for reading.\r\n", fileName);
	} else {
		printf("Failed to open file '%s' for reading!\r\n", fileName);
		return;
	}

	// 파일 전체 읽기
	memset(buffer, 0, sizeof(buffer));
	fres = f_read(&fil, buffer, sizeof(buffer) - 1, &br);

	if (fres == FR_OK && br > 0) {
		printf("-----------FILE CONTENT----------\r\n");
		printf("%s", buffer);
		if (buffer[strlen(buffer) - 1] != '\n')
			printf("\r\n");
		printf("-----------END OF FILE-----------\r\n");
		printf("%lu bytes read.\r\n", br);
	} else {
		printf("File is empty or read failed!\r\n");
	}

	f_close(&fil);  // 읽기 후 파일 닫기
}

// ========== 인터페이스 함수 =================

// 명령어 처리 함수
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
		char *filename = command + 5;  // "open " 다음 부분
		OpenFile(filename);
	}
	else if (strcmp(command, "close") == 0)
	{
		CloseFile();
	}
	else if (strncmp(command, "write ", 6) == 0)
	{
		char *text = command + 6;  // "write " 다음 부분
		WriteFile(text);
	} else if (strncmp(command, "read ", 5) == 0) {
		char *filename = command + 5;  // "read " 다음 부분
		ReadFile(filename);
	} else if (strncmp(command, "image ", 6) == 0) {  // 새로운 명령어 추가
        char *filename = command + 6;
        DisplayImageFromSD_Enhanced(filename);
	} else if (strcmp(command, "size") == 0) {
		CheckSize();
	} else if (strcmp(command, "help") == 0) {
		ShowHelp();
	} else if (strcmp(command, "antiflicker") == 0) {
	    OLED_setAntiFlicker();
	}
	else if (strcmp(command, "camera") == 0)
	{
		OLED_setCameraMode();
	} else {
		printf("Unknown command: %s\r\n", command);
		printf("Type 'help' for available commands.\r\n");
	}
}

// 도움말 함수
void ShowHelp(void) {
    printf("\r\n=== Available Commands ===\r\n");
    printf("mount              - Mount SD card\r\n");
    printf("unmount            - Unmount SD card\r\n");
    printf("open <filename>    - Open file\r\n");
    printf("close              - Close current file\r\n");
    printf("write <text>       - Write text to file\r\n");
    printf("read <filename>    - Read file content\r\n");
    printf("image <filename>   - Display BMP image on OLED\r\n");  // 추가
    printf("size               - Check SD card free space\r\n");
    printf("help               - Show this help\r\n");
    printf("==========================\r\n");
}

// SD카드에서 BMP 파일을 읽어 OLED에 출력하는 함수
void DisplayImageFromSD(char *fileName) {
    if (closeFlag == 0) {
        CloseFile();
    }

    fres = f_open(&fil, fileName, FA_READ);
    if (fres != FR_OK) {
        printf("Failed to open image file '%s'!\r\n", fileName);
        return;
    }

    // BMP 헤더 읽기
    uint8_t bmpHeader[54];
    fres = f_read(&fil, bmpHeader, 54, &br);
    if (fres != FR_OK || br < 54) {
        printf("Failed to read BMP header!\r\n");
        f_close(&fil);
        return;
    }

    // 이미지 정보 추출
    uint32_t dataOffset = *(uint32_t*)&bmpHeader[10];
    uint32_t width = *(uint32_t*)&bmpHeader[18];
    uint32_t height = *(uint32_t*)&bmpHeader[22];
    uint16_t bitsPerPixel = *(uint16_t*)&bmpHeader[28];

    printf("Image: %lux%lu, %d bits\r\n", width, height, bitsPerPixel);

    // 크기 체크
    if (width > 256 || height > 64) {
        printf("Image too large! Max: 256x64\r\n");
        f_close(&fil);
        return;
    }

    // 버퍼 클리어
    oled_clearBuffer();

    // 중앙 정렬 계산
    uint8_t startX = (256 - width) / 2;
    uint8_t startY = (64 - height) / 2;

    if (bitsPerPixel == 32) {
        // 32비트 처리 (기존 코드)
        uint32_t rowSize = width * 4;
        uint8_t *rowBuffer = (uint8_t*)malloc(rowSize);

        if (rowBuffer == NULL) {
            printf("Memory allocation failed!\r\n");
            f_close(&fil);
            return;
        }

        printf("Processing 32-bit image...\r\n");

        for (int row = 0; row < height; row++) {
            int bmpRow = height - 1 - row;
            uint32_t filePos = dataOffset + bmpRow * rowSize;

            f_lseek(&fil, filePos);
            fres = f_read(&fil, rowBuffer, rowSize, &br);

            if (fres != FR_OK || br < rowSize) break;

            for (uint32_t col = 0; col < width; col++) {
                uint32_t pixelOffset = col * 4;
                uint8_t b = rowBuffer[pixelOffset];
                uint8_t g = rowBuffer[pixelOffset + 1];
                uint8_t r = rowBuffer[pixelOffset + 2];

                uint16_t gray16 = (r * 299 + g * 587 + b * 114) / 1000;
                uint8_t gray = gray16 / 17;
                if (gray > 15) gray = 15;

                int displayX = startX + col;
                int displayY = startY + row;

                if (displayX >= 0 && displayX < 256 && displayY >= 0 && displayY < 64) {
                    oled_setPixelInBuffer(displayX, displayY, gray);
                }
            }
        }

        free(rowBuffer);

    } else if (bitsPerPixel == 24) {
        // 24비트 처리 (BGR, 3바이트/픽셀)
        printf("Processing 24-bit image...\r\n");

        // BMP 24비트의 행 크기는 4바이트 경계로 정렬됨
        uint32_t rowSize = ((width * 3 + 3) / 4) * 4;
        uint8_t *rowBuffer = (uint8_t*)malloc(rowSize);

        if (rowBuffer == NULL) {
            printf("Memory allocation failed!\r\n");
            f_close(&fil);
            return;
        }

        for (int row = 0; row < height; row++) {
            int bmpRow = height - 1 - row;
            uint32_t filePos = dataOffset + bmpRow * rowSize;

            f_lseek(&fil, filePos);
            fres = f_read(&fil, rowBuffer, rowSize, &br);

            if (fres != FR_OK || br < rowSize) {
                printf("Read error at row %d\r\n", row);
                break;
            }

            for (uint32_t col = 0; col < width; col++) {
                uint32_t pixelOffset = col * 3;  // 24비트는 3바이트/픽셀
                uint8_t b = rowBuffer[pixelOffset];
                uint8_t g = rowBuffer[pixelOffset + 1];
                uint8_t r = rowBuffer[pixelOffset + 2];

                // RGB를 그레이스케일로 변환 (0-15 범위)
                uint16_t gray16 = (r * 299 + g * 587 + b * 114) / 1000;
                uint8_t gray = gray16 / 17;
                if (gray > 15) gray = 15;

                int displayX = startX + col;
                int displayY = startY + row;

                if (displayX >= 0 && displayX < 256 && displayY >= 0 && displayY < 64) {
                    oled_setPixelInBuffer(displayX, displayY, gray);
                }
            }

            // 진행상황 출력 (10행마다)
            if (row % 10 == 0) {
                printf("Processing row %d/%lu\r\n", row, height);
            }
        }

        free(rowBuffer);

    } else if (bitsPerPixel == 1) {
        // 1비트 처리
        printf("Processing 1-bit image...\r\n");

        uint32_t rowSize = (width + 7) / 8;
        uint32_t imageSize = rowSize * height;

        uint8_t *imageData = (uint8_t*)malloc(imageSize);
        if (imageData == NULL) {
            printf("Memory allocation failed!\r\n");
            f_close(&fil);
            return;
        }

        f_lseek(&fil, dataOffset);
        fres = f_read(&fil, imageData, imageSize, &br);

        if (fres == FR_OK && br == imageSize) {
            for (uint16_t row = 0; row < height; row++) {
                for (uint16_t col = 0; col < width; col++) {
                    uint32_t byteIndex = (height - 1 - row) * rowSize + (col / 8);
                    uint8_t bitIndex = 7 - (col % 8);

                    uint8_t gray = (imageData[byteIndex] & (1 << bitIndex)) ? 0 : 15;

                    int displayX = startX + col;
                    int displayY = startY + row;

                    if (displayX >= 0 && displayX < 256 && displayY >= 0 && displayY < 64) {
                        oled_setPixelInBuffer(displayX, displayY, gray);
                    }
                }
            }
        }

        free(imageData);

    } else {
        printf("Unsupported bit depth: %d\r\n", bitsPerPixel);
        f_close(&fil);
        return;
    }

    // 한 번에 전체 화면 업데이트
    oled_updateDisplay();

    printf("Image displayed successfully!\r\n");
    f_close(&fil);
}
uint8_t enhanceGrayscale(uint8_t r, uint8_t g, uint8_t b) {
    // RGB를 그레이스케일로 변환
    uint16_t gray = (r * 299 + g * 587 + b * 114) / 1000;

    // 히스토그램 스트레칭 (명암 대비 향상)
    if (gray < 64) {
        // 어두운 부분을 더 어둡게
        return (gray * 6) / 64;  // 0-6 범위
    } else if (gray < 128) {
        // 중간 톤
        return 6 + ((gray - 64) * 4) / 64;  // 6-10 범위
    } else if (gray < 192) {
        // 밝은 중간 톤
        return 10 + ((gray - 128) * 3) / 64;  // 10-13 범위
    } else {
        // 매우 밝은 부분을 더 밝게
        return 13 + ((gray - 192) * 2) / 63;  // 13-15 범위
    }
}// 감마 보정 테이블 (미리 계산된 값들)
const uint8_t gammaTable[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6,
    6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8,
    8, 8, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 11, 11,
    11, 11, 11, 11, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 14, 14,
    14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15
};

uint8_t gammaCorrectGrayscale(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t gray = (r * 299 + g * 587 + b * 114) / 1000;
    return gammaTable[gray];
}
// 이미지를 두 번 읽는 방식: 첫 번째는 히스토그램 분석, 두 번째는 실제 출력
void DisplayImageFromSD_Enhanced(char *fileName) {
    if (closeFlag == 0) {
        CloseFile();
    }

    fres = f_open(&fil, fileName, FA_READ);
    if (fres != FR_OK) {
        printf("Failed to open image file '%s'!\r\n", fileName);
        return;
    }

    // BMP 헤더 읽기 (기존과 동일)
    uint8_t bmpHeader[54];
    fres = f_read(&fil, bmpHeader, 54, &br);
    if (fres != FR_OK || br < 54) {
        printf("Failed to read BMP header!\r\n");
        f_close(&fil);
        return;
    }

    uint32_t dataOffset = *(uint32_t*)&bmpHeader[10];
    uint32_t width = *(uint32_t*)&bmpHeader[18];
    uint32_t height = *(uint32_t*)&bmpHeader[22];
    uint16_t bitsPerPixel = *(uint16_t*)&bmpHeader[28];

    printf("Image: %lux%lu, %d bits\r\n", width, height, bitsPerPixel);

    if (width > 256 || height > 64) {
        printf("Image too large! Max: 256x64\r\n");
        f_close(&fil);
        return;
    }

    // 32비트 처리 (24비트도 유사하게 수정)
    if (bitsPerPixel == 32) {
        uint32_t rowSize = width * 4;
        uint8_t *rowBuffer = (uint8_t*)malloc(rowSize);

        if (rowBuffer == NULL) {
            printf("Memory allocation failed!\r\n");
            f_close(&fil);
            return;
        }

        // 1단계: 히스토그램 분석 (최소/최대 밝기 찾기)
        uint8_t minGray = 255, maxGray = 0;

        printf("Analyzing image histogram...\r\n");
        for (int row = 0; row < height; row++) {
            int bmpRow = height - 1 - row;
            uint32_t filePos = dataOffset + bmpRow * rowSize;

            f_lseek(&fil, filePos);
            fres = f_read(&fil, rowBuffer, rowSize, &br);
            if (fres != FR_OK) break;

            for (uint32_t col = 0; col < width; col++) {
                uint32_t pixelOffset = col * 4;
                uint8_t b = rowBuffer[pixelOffset];
                uint8_t g = rowBuffer[pixelOffset + 1];
                uint8_t r = rowBuffer[pixelOffset + 2];

                uint8_t gray = (r * 299 + g * 587 + b * 114) / 1000;
                if (gray < minGray) minGray = gray;
                if (gray > maxGray) maxGray = gray;
            }
        }

        printf("Gray range: %d - %d\r\n", minGray, maxGray);

        // 2단계: 히스토그램 스트레칭을 적용하여 실제 출력
        oled_clearBuffer();

        uint8_t startX = (256 - width) / 2;
        uint8_t startY = (64 - height) / 2;

        printf("Rendering with enhanced contrast...\r\n");
        for (int row = 0; row < height; row++) {
            int bmpRow = height - 1 - row;
            uint32_t filePos = dataOffset + bmpRow * rowSize;

            f_lseek(&fil, filePos);
            fres = f_read(&fil, rowBuffer, rowSize, &br);
            if (fres != FR_OK) break;

            for (uint32_t col = 0; col < width; col++) {
                uint32_t pixelOffset = col * 4;
                uint8_t b = rowBuffer[pixelOffset];
                uint8_t g = rowBuffer[pixelOffset + 1];
                uint8_t r = rowBuffer[pixelOffset + 2];

                uint8_t gray = (r * 299 + g * 587 + b * 114) / 1000;

                // 히스토그램 스트레칭 적용
                uint8_t enhanced;
                if (maxGray > minGray) {
                    enhanced = ((gray - minGray) * 15) / (maxGray - minGray);
                } else {
                    enhanced = 8;  // 단색 이미지인 경우 중간값
                }

                if (enhanced > 15) enhanced = 15;

                int displayX = startX + col;
                int displayY = startY + row;

                if (displayX >= 0 && displayX < 256 && displayY >= 0 && displayY < 64) {
                    oled_setPixelInBuffer(displayX, displayY, enhanced);
                }
            }
        }

        free(rowBuffer);
    }

    oled_updateDisplay();
    printf("Enhanced image displayed!\r\n");
    f_close(&fil);
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
  if (MX_FATFS_Init() != APP_OK) {
    Error_Handler();
  }
  MX_SPI3_Init();
  /* USER CODE BEGIN 2 */

	OLED_init();
	OLED_fill(0);

	static uint8_t rxData;
	HAL_UART_Receive_IT(&huart1, &rxData, 1);

	printf("\r\n=== SD Card Control System ===\r\n");
	printf("Type 'help' for available commands.\r\n");
	printf("Ready> ");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {

		uint8_t currentCardState = SD_IsCardDetected();

		if (currentCardState && !previousCardState) {
			// 카드 삽입 감지
			HAL_Delay(500); // 디바운싱
			SDMount();
		} else if (!currentCardState && previousCardState) {
			// 카드 제거 감지
			SDUnmount();
		}

		previousCardState = currentCardState;

		// 명령어 처리
		if (commandReady) {
			ProcessCommand(rxBuffer);
			commandReady = 0;
			printf("Ready> ");
		}

		HAL_Delay(100);

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
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1) {
		static uint8_t rxData;

		if (rxData == '\r' || rxData == '\n')  // Enter 키 감지
				{
			rxBuffer[rxIndex] = '\0';  // 문자열 종료
			commandReady = 1;
			rxIndex = 0;
		} else if (rxIndex < sizeof(rxBuffer) - 1) {
			rxBuffer[rxIndex++] = rxData;
		}

		// 다음 문자 수신 대기
		HAL_UART_Receive_IT(&huart1, &rxData, 1);
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
