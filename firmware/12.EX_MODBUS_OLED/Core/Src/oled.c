#include "oled.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// 초기화용 변수
static uint8_t temp = 0;
extern int8_t closeFlag;
static uint8_t frameBuffer[OLED_W/2 * OLED_H];  // 전체 화면 버퍼

/**
 * @brief SPI 전송 Low-level
 *
 */

// 명령어 1바이트 전송
static void OLED_write_cmd(uint8_t cmd)
{
  DC_CMD();                                         // DC=0, 명령 모드
  CS_L();                                           // CS=0, OLED 칩 선택
  HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY); // SPI로 1바이트 명령 전송
  CS_H();                                           // CS=1, 전송 끝
}

// 데이터 다중 바이트 전송
static void OLED_write_data(const uint8_t *p, uint16_t len)
{
  DC_DATA();                                                  // DC=1, 데이터 모드
  CS_L();                                                     // CS=0, 칩 선택
  HAL_SPI_Transmit(&hspi3, (uint8_t *)p, len, HAL_MAX_DELAY); // 데이터 블록 전송
  CS_H();                                                     // CS=1, 전송 끝
}

/**
 * @brief 디스플레이 윈도우 설정
 * - SSD1322는 "컬럼 범위"와 "행 범위"를 따로 지정해야 한다.
 * - 이후 WRITERAM(0x5C) 명령을 주면 해당 영역에 데이터를 순서대로 써진다.
 */

static void OLED_set_window(uint8_t col_start, uint8_t col_end,
                            uint8_t row_start, uint8_t row_end)
{
  // Column (가로 영역) 지정
  OLED_write_cmd(OLED_SETCOLUMNADDR);
  uint8_t col[2] = {col_start, col_end};
  OLED_write_data(col, 2);

  // Row (세로 영역) 지정
  OLED_write_cmd(OLED_SETROWADDR);
  uint8_t row[2] = {row_start, row_end};
  OLED_write_data(row, 2);
}

/**
 * @brief OLED 초기화 함수
 * - 전원 인가 후 반드시 필요한 설정 시퀀스
 * - 데이터시트에 나온 권장 초기화 값들을 대부분 포함
 */

void OLED_init(void)
{
  // 0. 하드웨어 리셋
  RST_L();
  HAL_Delay(10);
  RST_H();
  HAL_Delay(10);

  // 1. 화면 끄기 (설정은 반드시 OFF 상태에서 해야 함)
  OLED_write_cmd(OLED_DISPLAYOFF);

  // 2. 명령어 잠금 해제 (모든 명령 사용 가능)
  OLED_write_cmd(OLED_SETCOMMANDLOCK);
  temp = 0x12;
  OLED_write_data(&temp, 1);

  // 3. 클럭 설정 (화면 동작 속도 관련)
  OLED_write_cmd(OLED_SETCLOCKDIVIDER);
  temp = 0x91; // 분주비=1, 오실레이터=9
  OLED_write_data(&temp, 1);

  // 4. 세로 해상도 (라인 수 = 64)
  OLED_write_cmd(OLED_SETMUXRATIO);
  temp = 0x3F; // 63+1 = 64라인
  OLED_write_data(&temp, 1);

  // 5. 화면 세로 오프셋 (위/아래로 밀기)
  OLED_write_cmd(OLED_SETDISPLAYOFFSET);
  temp = 0x00; // 안 밀기
  OLED_write_data(&temp, 1);

  // 6. 시작 라인 (RAM에서 0번 라인부터 출력)
  OLED_write_cmd(OLED_SETSTARTLINE);
  temp = 0x00;
  OLED_write_data(&temp, 1);

  // 7. 화면 배치 (좌우/상하 반전, nibble 스왑 등)
  OLED_write_cmd(OLED_SETREMAP);
  uint8_t remap[2] = {0x14, 0x11}; // 보편적인 설정
  OLED_write_data(remap, 2);

  // 8. GPIO 비활성화 (안 씀)
  OLED_write_cmd(OLED_SETGPIO);
  temp = 0x00;
  OLED_write_data(&temp, 1);

  // 9. 내부 전원(VDD) 사용
  OLED_write_cmd(OLED_FUNCTIONSELECT);
  temp = 0x01;
  OLED_write_data(&temp, 1);

  // 10. 화면 향상 A (화질 최적화)
  OLED_write_cmd(OLED_DISPLAYENHANCE_A);
  uint8_t enhA[2] = {0xA0, 0xFD};
  OLED_write_data(enhA, 2);

  // 11. 밝기 (세그먼트 전류, 0~255)
  OLED_write_cmd(OLED_SETCONTRAST);
  temp = 0xFF; // 최대 밝기
  OLED_write_data(&temp, 1);

  // 12. 밝기 전체 스케일 (0~15)
  OLED_write_cmd(OLED_MASTERCURRENT);
  temp = 0x0F; // 최대
  OLED_write_data(&temp, 1);

  // 13. 기본 그레이스케일 테이블 사용
  OLED_write_cmd(OLED_DEFAULTGRAYSCALE);

  // 14. 신호 타이밍 (화면 깜빡임/잔상 관련)
  OLED_write_cmd(OLED_SETPHASELENGTH);
  temp = 0xE2;
  OLED_write_data(&temp, 1);

  // 15. 화면 향상 B
  OLED_write_cmd(OLED_DISPLAYENHANCE_B);
  uint8_t enhB[2] = {0x82, 0x20};
  OLED_write_data(enhB, 2);

  // 16. 프리차지 전압
  OLED_write_cmd(OLED_SETPRECHARGEVOLTAGE);
  temp = 0x1F;
  OLED_write_data(&temp, 1);

  // 17. 두 번째 프리차지 기간
  OLED_write_cmd(OLED_SETSECONDPRECHARGE);
  temp = 0x08;
  OLED_write_data(&temp, 1);

  // 18. 출력 전압 범위 (VCOMH)
  OLED_write_cmd(OLED_SETVCOMH);
  temp = 0x07;
  OLED_write_data(&temp, 1);

  // 19. 일반 디스플레이 모드
  OLED_write_cmd(OLED_NORMALDISPLAY);

  // 20. 부분 모드 해제
  OLED_write_cmd(OLED_EXITPARTIALDISPLAY);

  // 21. 전체 화면 영역 지정 (256x64)
  OLED_set_window(OLED_COL_START, OLED_COL_END, OLED_ROW_START, OLED_ROW_END);

  // 22. 화면 켜기
  OLED_write_cmd(OLED_DISPLAYON);
  HAL_Delay(50);
}

/**
 * @brief 화면 전체를 단색으로 채우는 함수
 * - gray : 0~15 (4비트 값, 0=검정 ~ 15=흰색)
 */

void OLED_fill(uint8_t gray)
{
  // gray 값을 상위/하위 nibble 모두 채운 1바이트 패턴으로 변환
  uint8_t grayed = (gray << 4) | gray;

  // 한 줄 데이터 (256픽셀 / 2 = 128바이트)
  uint8_t line[OLED_W / 2];
  memset(line, grayed, sizeof(line));

  // 전체 영역 지정
  OLED_set_window(OLED_COL_START, OLED_COL_END, OLED_ROW_START, OLED_ROW_END);

  // RAM 쓰기 시작
  OLED_write_cmd(OLED_WRITERAM);

  // 64줄 반복해서 같은 라인 데이터를 써 넣음
  for (int y = 0; y < OLED_H; y++)
  {
    OLED_write_data(line, sizeof(line));
  }
}

/**
 * @brief 좌표 관련 함수
 */

// 픽셀의 x좌표 -> OLED의 column 바이트 주소
static uint8_t COL_from_x(int x)
{
  // 4bpp에서는 2픽셀당 1바이트이므로 x를 2로 나눔
  return OLED_COL_START + (x >> 1); // x >> 1은 x / 2와 같음
}

// 픽셀의 y좌표를 OLED의 row 주소로 변환
static uint8_t ROW_from_y(int y)
{
  return OLED_ROW_START + y; // y좌표는 그대로 사용
}

/**
 * @brief 문자 출력 함수
 */

// 하나의 문자를 화면에 그리는 함수
void OLED_drawChar(int x, int y, char ch, const FontDef *font, uint8_t gray)
{
  // 1단계: 입력값 검증
  if (ch < 32 || ch > 126)
  {
    return; // 출력 가능한 ASCII 문자만 허용
  }
  if (gray > 15)
  {
    gray = 15; // 최대 밝기 제한
  }

  // 2단계: 화면 범위 검사
  if (x < 0 || (x + font->width) > OLED_W)
  {
    return; // 가로 범위 초과
  }

  if (y < 0 || (y + font->height) > OLED_H)
  {
    return; // 세로 범위 초과
  }

  // 3단계: 폰트 데이터에서 문자 찾기
  const int stride = font->height;                 // 한 문자 데이터 줄 수
  const int start = (ch - 32) * stride;            // 문자 데이터 시작 위치 ('A'는 65-32=33번째)
  const int bytes_per_row = (font->width + 1) / 2; // 한 줄당 필요한 바이트 수

  uint8_t linebuf[OLED_W / 2]; // 한 줄 데이터를 저장할 버퍼

  // 4단계: 문자의 각 줄을 처리
  for (int row = 0; row < font->height; row++)
  {
    // 폰트 데이터에서 현재 줄의 비트맵 가져오기
    uint16_t mask = font->data[start + row];

    // 줄 버퍼를 0으로 초기화 (배경색)
    memset(linebuf, 0x00, bytes_per_row);

    // 5단계: 각 픽셀을 처리 (왼쪽부터 오른쪽으로)
    for (int col = 0; col < font->width; col++)
    {
      int byte_idx = (col >> 1);          // 현재 픽셀이 들어갈 바이트 위치
      int left_nibble = ((col & 1) == 0); // 바이트의 왼쪽 4bit인지 오른쪽 4bit인지

      // 현재 픽셀이 켜져있는지 확인
      if (mask & (0x8000 >> col)) // 0x8000에서 오른쪽으로 col번 이동한 비트 확인
      {
        // 픽셀이 켜져있으면 해당 위치에 색상 설정
        if (left_nibble)
        {
          linebuf[byte_idx] |= (gray << 4); // 왼쪽 4bit에 색상
        }
        else
        {
          linebuf[byte_idx] |= (gray & 0x0F); // 오른쪽 4bit에 색상
        }
      }
    }

    // 6단계: 현재 줄을 OLED에 전송
    uint8_t col_start = COL_from_x(x);               // 시작 column 주소
    uint8_t col_end = col_start + bytes_per_row - 1; // 끝 column 주소
    uint8_t row_addr = ROW_from_y(y + row);          // 현재 row 주소

    OLED_set_window(col_start, col_end, row_addr, row_addr); // 그리기 영역 설정
    OLED_write_cmd(OLED_WRITERAM);                           // 메모리 쓰기 시작
    OLED_write_data(linebuf, bytes_per_row);                 // 데이터 전송
  }
}

/* 문자열을 화면에 그리는 함수 */
void OLED_drawString(int x, int y, const char *str, const FontDef *font, uint8_t gray)
{
  int c_x = x;
  int c_y = y; // 현재 커서 위치

  // 문자열의 끝까지 반복
  while (*str)
  {
    if (*str == '\n')
    {
      c_y += font->height + 1; // 다음 줄로 이동 (+1은 줄 간격)
      c_x = x;                 // 맨 왼쪽으로 돌아가기
    }

    else if (*str != '\r')
    {
      OLED_drawChar(c_x, c_y, *str, font, gray); // 현재 위치에 문자 그리기
      c_x += 4;                                 // 다음 문자 위치로 이동 (4픽셀 간격으로 설정)
    }
    str++; // 다음 문자로 이동
  }
}

/* ======== BMP 이미지 출력 함수들 ======== */

/* 개별 픽셀을 그리는 함수 */
void oled_drawPixel(int x, int y, uint8_t gray)
{
    if (x < 0 || x >= OLED_W || y < 0 || y >= OLED_H) {
        return;
    }

    if (gray > 15) {
        gray = 15;
    }

    uint8_t col_byte = colbyte_from_x(x);
    uint8_t row_addr = rowaddr_from_y(y);

    // 현재 바이트의 기존 값을 읽어올 수 없으므로
    // 인접한 두 픽셀을 함께 처리하는 방식으로 변경해야 함

    // 임시방편: 같은 그레이스케일 값으로 양쪽 니블 모두 설정
    uint8_t pixel_data = (gray << 4) | (gray & 0x0F);

    OLED_set_window(col_byte, col_byte, row_addr, row_addr);
    OLED_write_cmd(OLED_WRITERAM);
    OLED_write_data(&pixel_data, 1);
}

/* BMP 이미지 데이터를 OLED에 출력하는 함수 */
void oled_drawBitmap(uint8_t *bmpData, uint16_t width, uint16_t height, uint8_t x, uint8_t y)
{
    uint32_t rowSize = (width + 7) / 8;  // BMP 행 크기 (바이트 단위)

    // BMP는 아래쪽부터 저장되므로 뒤집어서 출력
    for (uint16_t row = 0; row < height; row++)
    {
        for (uint16_t col = 0; col < width; col++)
        {
            // 경계 체크
            if ((x + col) >= OLED_W || (y + row) >= OLED_H)
            {
                continue;
            }

            uint32_t byteIndex = (height - 1 - row) * rowSize + (col / 8);
            uint8_t bitIndex = 7 - (col % 8);

            // 해당 픽셀이 검은색인지 확인 (BMP에서 0=검정, 1=흰색)
            if (!(bmpData[byteIndex] & (1 << bitIndex)))
            {
                // OLED에 픽셀 그리기 (검은색을 15(흰색)로, 흰색을 0(검정)으로)
                oled_drawPixel(x + col, y + row, 15);
            }
            else
            {
                oled_drawPixel(x + col, y + row, 0);
            }
        }
    }
}

/* 이미지를 중앙에 출력하는 함수 */
void oled_drawBitmapCenter(uint8_t *bmpData, uint16_t width, uint16_t height)
{
    // 중앙 정렬 계산
    uint8_t startX = (OLED_W - width) / 2;
    uint8_t startY = (OLED_H - height) / 2;

    // 화면 클리어
    OLED_fill(0);

    // 이미지 그리기
    oled_drawBitmap(bmpData, width, height, startX, startY);
}


void oled_setPixelInBuffer(int x, int y, uint8_t gray)
{
    if (x < 0 || x >= OLED_W || y < 0 || y >= OLED_H) {
        return;
    }

    if (gray > 15) {
        gray = 15;
    }

    int bufferIndex = y * (OLED_W/2) + (x/2);

    if (x & 1) {
        // 홀수 x: 하위 니블
        frameBuffer[bufferIndex] = (frameBuffer[bufferIndex] & 0xF0) | (gray & 0x0F);
    } else {
        // 짝수 x: 상위 니블
        frameBuffer[bufferIndex] = (frameBuffer[bufferIndex] & 0x0F) | (gray << 4);
    }
}

void oled_updateDisplay(void)
{
    OLED_set_window(OLED_COL_START, OLED_COL_END, OLED_ROW_START, OLED_ROW_END);
    OLED_write_cmd(OLED_WRITERAM);
    OLED_write_data(frameBuffer, sizeof(frameBuffer));
}

void oled_clearBuffer(void)
{
    memset(frameBuffer, 0x00, sizeof(frameBuffer));
}


// SD카드에서 BMP 파일을 읽어 OLED에 출력하는 함수
void displayImageFromSD(char *fileName)
{
    if (closeFlag == 0)
    {
        closeFile();
    }

    fres = f_open(&fil, fileName, FA_READ);
    if (fres != FR_OK)
    {
        printf("Failed to open image file '%s'!\r\n", fileName);
        return;
    }

    // BMP 헤더 읽기 (기존과 동일)
    uint8_t bmpHeader[54];
    fres = f_read(&fil, bmpHeader, 54, &br);
    if (fres != FR_OK || br < 54)
    {
        printf("Failed to read BMP header!\r\n");
        f_close(&fil);
        return;
    }

    uint32_t dataOffset = *(uint32_t*)&bmpHeader[10];
    uint32_t width = *(uint32_t*)&bmpHeader[18];
    uint32_t height = *(uint32_t*)&bmpHeader[22];
    uint16_t bitsPerPixel = *(uint16_t*)&bmpHeader[28];

    printf("Image: %lux%lu, %d bits\r\n", width, height, bitsPerPixel);

    if (width > 256 || height > 64)
    {
        printf("Image too large! Max: 256x64\r\n");
        f_close(&fil);
        return;
    }

    // 32비트 처리 (24비트도 유사하게 수정)
    if (bitsPerPixel == 32)
    {
        uint32_t rowSize = width * 4;
        uint8_t *rowBuffer = (uint8_t*)malloc(rowSize);

        if (rowBuffer == NULL)
        {
            printf("Memory allocation failed!\r\n");
            f_close(&fil);
            return;
        }

        // 1단계: 히스토그램 분석 (최소/최대 밝기 찾기)
        uint8_t minGray = 255, maxGray = 0;

        printf("Analyzing image histogram...\r\n");
        for (int row = 0; row < height; row++)
        {
            int bmpRow = height - 1 - row;
            uint32_t filePos = dataOffset + bmpRow * rowSize;

            f_lseek(&fil, filePos);
            fres = f_read(&fil, rowBuffer, rowSize, &br);
            if (fres != FR_OK) break;

            for (uint32_t col = 0; col < width; col++)
            {
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
            if (fres != FR_OK)
            {
            	break;
            }

            for (uint32_t col = 0; col < width; col++)
            {
                uint32_t pixelOffset = col * 4;
                uint8_t b = rowBuffer[pixelOffset];
                uint8_t g = rowBuffer[pixelOffset + 1];
                uint8_t r = rowBuffer[pixelOffset + 2];

                uint8_t gray = (r * 299 + g * 587 + b * 114) / 1000;

                // 히스토그램 스트레칭 적용
                uint8_t enhanced;
                if (maxGray > minGray)
                {
                    enhanced = ((gray - minGray)  * 15) / (maxGray - minGray);
                }
                else
                {
                    enhanced = 8;  // 단색 이미지인 경우 중간값
                }

                if (enhanced > 15)
                {
                	enhanced = 15;
                }

                int displayX = startX + col;
                int displayY = startY + row;

                if (displayX >= 0 && displayX < 256 && displayY >= 0 && displayY < 64)
                {
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

/* 촬영 모드 - 플리커 최소화 */
void OLED_setCameraMode(void)
{
    // 촬영을 위한 최적 설정 (밝기 유지하면서 플리커 감소)
    OLED_write_cmd(OLED_SETCLOCKDIVIDER);
    temp = 0x70;  // 느린 클록 (플리커 감소)
    OLED_write_data(&temp, 1);
    OLED_write_cmd(OLED_MASTERCURRENT);
    temp = 0x0F;  // 0x08에서 0x0D로 증가 (밝기 유지)
    OLED_write_data(&temp, 1);

    // 대비도 최대로
       OLED_write_cmd(OLED_SETCONTRAST);
       temp = 0xFF;
       OLED_write_data(&temp, 1);

    HAL_Delay(200);
    printf("Camera mode enabled - balanced brightness and flicker\r\n");
}
