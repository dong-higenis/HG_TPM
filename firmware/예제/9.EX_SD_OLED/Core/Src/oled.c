#include "oled.h"
#include <string.h>   // memset 함수 사용을 위해 필요

// 라이브러리 처럼 쓰시면 됩니다.

/* 내부에서만 사용하는 임시 변수 (static) */
static uint8_t d = 0;
extern closeFlag;

// OLED 명령어 모드!
static void OLED_write_cmd(uint8_t cmd)
{
  DC_CMD();     // DC핀을 LOW로 설정 (명령어 모드)
  CS_L();       // CS핀을 LOW로 설정 (통신 시작)
  HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY);  // SPI로 명령어 전송
  CS_H();       // CS핀을 HIGH로 설정 (통신 종료)
}

// OLED 데이터 모드!
static void OLED_write_data(const uint8_t* p, uint16_t len)
{
  DC_DATA();    // DC핀을 HIGH로 설정 (데이터 모드)
  CS_L();       // CS핀을 LOW로 설정 (통신 시작)
  HAL_SPI_Transmit(&hspi3, (uint8_t*)p, len, HAL_MAX_DELAY);  // SPI로 데이터 전송
  CS_H();       // CS핀을 HIGH로 설정 (통신 종료)
}

// 화면의 특정 영역을 선택하는 함수 (그리기 영역 설정)
static void OLED_set_window(uint8_t col_start, uint8_t col_end,
                            uint8_t row_start, uint8_t row_end)
{
  // 열(가로) 범위 설정
  OLED_write_cmd(OLED_SETCOLUMNADDR);
  uint8_t col[2] = { col_start, col_end };
  OLED_write_data(col, 2);

  // 행(세로) 범위 설정
  OLED_write_cmd(OLED_SETROWADDR);
  uint8_t row[2] = { row_start, row_end };
  OLED_write_data(row, 2);
}


/* OLED 디스플레이를 초기화하는 함수 */
/* OLED 디스플레이를 초기화하는 함수 (플리커 방지 버전) */
void OLED_init(void)
{
  RST_L();
  HAL_Delay(10);
  RST_H();
  HAL_Delay(10);

  OLED_write_cmd(OLED_DISPLAYOFF);

  // 명령어 락 설정 - 더 안정적인 값
  OLED_write_cmd(OLED_SETCOMMANDLOCK);
  d = 0x12;  // 0x80 대신 0x12 사용 (표준값)
  OLED_write_data(&d, 1);

  // 클록 디바이더 - 플리커 방지를 위한 핵심 설정
  OLED_write_cmd(OLED_SETCLOCKDIVIDER);
  d = 0x80;  // 0x91 대신 0x80 사용 (더 느린 클록으로 플리커 감소)
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_SETMUXRATIO);
  d = 0x3F;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_SETDISPLAYOFFSET);
  d = 0x00;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_SETSTARTLINE);
  d = 0x00;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_SETREMAP);
  {
    uint8_t remap[2] = {0x06, 0x11};
    OLED_write_data(remap, 2);
  }

  OLED_write_cmd(OLED_SETGPIO);
  d = 0x00;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_FUNCTIONSELECT);
  d = 0x01;
  OLED_write_data(&d, 1);

  // 디스플레이 인핸스 A - 플리커 방지 설정
  OLED_write_cmd(OLED_DISPLAYENHANCE_A);
  {
    uint8_t enhA[2] = {0xA0, 0xFD};  // 0xB5 대신 0xFD 사용 (더 안정적)
    OLED_write_data(enhA, 2);
  }

  // 대비 설정 - 조금 낮춤
  OLED_write_cmd(OLED_SETCONTRAST);
  d = 0xFF;  // 0xFF 대신 0xDF 사용 (과도한 밝기로 인한 플리커 방지)
  OLED_write_data(&d, 1);

  // 마스터 전류 - 조금 낮춤
  OLED_write_cmd(OLED_MASTERCURRENT);
  d = 0x0F;  // 0x0F 대신 0x0C 사용 (전류 감소로 플리커 방지)
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_DEFAULTGRAYSCALE);

  // 위상 길이 - 플리커 방지를 위한 핵심 설정
  OLED_write_cmd(OLED_SETPHASELENGTH);
  d = 0xC8;  // 0xE2 대신 0xC8 사용 (더 짧은 위상으로 플리커 감소)
  OLED_write_data(&d, 1);

  // 디스플레이 인핸스 B - 플리커 방지 설정
  OLED_write_cmd(OLED_DISPLAYENHANCE_B);
  {
    uint8_t enhB[2] = {0x20, 0x00};  // {0x82, 0x20} 대신 더 보수적인 값
    OLED_write_data(enhB, 2);
  }

  // 프리차지 전압 - 낮춤
  OLED_write_cmd(OLED_SETPRECHARGEVOLTAGE);
  d = 0x17;  // 0x1F 대신 0x17 사용
  OLED_write_data(&d, 1);

  // 세컨드 프리차지 - 길게 설정
  OLED_write_cmd(OLED_SETSECONDPRECHARGE);
  d = 0x0F;  // 0x08 대신 0x0F 사용 (더 긴 프리차지)
  OLED_write_data(&d, 1);

  // VCOMH 설정 - 조정
  OLED_write_cmd(OLED_SETVCOMH);
  d = 0x05;  // 0x07 대신 0x05 사용
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_NORMALDISPLAY);
  OLED_write_cmd(OLED_EXITPARTIALDISPLAY);

  OLED_set_window(OLED_COL_START, OLED_COL_END, OLED_ROW_START, OLED_ROW_END);

  OLED_write_cmd(OLED_DISPLAYON);
  HAL_Delay(100);  // 50ms 대신 100ms로 더 긴 안정화 시간
}


/* 화면 전체를 하나의 색으로 채우는 함수 */
void OLED_fill(uint8_t gray)  // gray: 0(검정)~15(흰색)
{

  uint8_t b = (gray << 4) | (gray & 0x0F);
  uint8_t line[OLED_W/2];
  memset(line, b, sizeof(line));

  // 전체 화면을 그리기 영역으로 설정
  OLED_set_window(OLED_COL_START, OLED_COL_END, OLED_ROW_START, OLED_ROW_END);
  OLED_write_cmd(OLED_WRITERAM);

  // 64줄을 반복해서 같은 데이터 전송
  for (int y = 0; y < OLED_H; y++) {
    OLED_write_data(line, sizeof(line));
  }
}

/* ======== 좌표 변환 함수들 ======== */

/* 픽셀의 x좌표를 OLED의 column 바이트 주소로 변환 */
static uint8_t colbyte_from_x(int x)
{

  return OLED_COL_START + (x >> 1);
}

/* 픽셀의 y좌표를 OLED의 row 주소로 변환 */
static uint8_t rowaddr_from_y(int y)
{
  return OLED_ROW_START + y;
}

/* ======== 문자 그리기 함수들 ======== */

/* 하나의 문자를 화면에 그리는 함수 */
void oled_drawChar(int x, int y, char ch, const FontDef *font, uint8_t gray)
{

  if (ch < 32 || ch > 126)
	  {
	  	  return;
	  }

  if (gray > 15)
	  {
	  	  gray = 15;
	  }


  if (x < 0 || (x + font->width) > OLED_W)
	  {
	  	  return;
	  }

  if (y < 0 || (y + font->height) > OLED_H)
	  {
	  	  return;
	  }


  const int stride = font->height;
  const int start  = (ch - 32) * stride;
  const int bytes_per_row = (font->width + 1) / 2;

  uint8_t linebuf[OLED_W/2];

  for (int row = 0; row < font->height; row++)
  {

    uint16_t mask = font->data[start + row];
    memset(linebuf, 0x00, bytes_per_row);

    for (int col = 0; col < font->width; col++)
    {
      int byte_idx = (col >> 1);
      int left_nibble = ((col & 1) == 0);

      if (mask & (0x8000 >> col))
      {
        if (left_nibble)
          linebuf[byte_idx] |= (gray << 4);
        else{
        		linebuf[byte_idx] |= (gray & 0x0F);
        	}
      }
    }

    uint8_t col_start = colbyte_from_x(x);
    uint8_t col_end   = col_start + bytes_per_row - 1;
    uint8_t row_addr  = rowaddr_from_y(y + row);

    OLED_set_window(col_start, col_end, row_addr, row_addr);
    OLED_write_cmd(OLED_WRITERAM);
    OLED_write_data(linebuf, bytes_per_row);
  }
}

/* 문자열을 화면에 그리는 함수 */
void oled_drawString(int x, int y, const char *str, const FontDef *font, uint8_t gray)
{
  int cx = x, cy = y;

  while (*str) {
    if (*str == '\n')
    {
      cy += font->height + 1;
      cx = x;
    }

    else if (*str != '\r')
    {
      oled_drawChar(cx, cy, *str, font, gray);
      cx += 4;
    }
    str++;  // 다음 문자로 이동
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


/* 화면 업데이트 함수 (필요시 사용) */
void oled_update(void)
{
    // 이미 각 픽셀이 바로 업데이트되므로 여기서는 딱히 할 일 없음
    // 필요하다면 전체 화면 리프레시 로직 추가 가능
}

/* BMP 이미지 데이터를 OLED에 출력하는 함수 */
void oled_drawBitmap(uint8_t *bmpData, uint16_t width, uint16_t height, uint8_t x, uint8_t y)
{
    uint32_t rowSize = (width + 7) / 8;  // BMP 행 크기 (바이트 단위)

    // BMP는 아래쪽부터 저장되므로 뒤집어서 출력
    for (uint16_t row = 0; row < height; row++) {
        for (uint16_t col = 0; col < width; col++) {
            // 경계 체크
            if ((x + col) >= OLED_W || (y + row) >= OLED_H) {
                continue;
            }

            uint32_t byteIndex = (height - 1 - row) * rowSize + (col / 8);
            uint8_t bitIndex = 7 - (col % 8);

            // 해당 픽셀이 검은색인지 확인 (BMP에서 0=검정, 1=흰색)
            if (!(bmpData[byteIndex] & (1 << bitIndex))) {
                // OLED에 픽셀 그리기 (검은색을 15(흰색)로, 흰색을 0(검정)으로)
                oled_drawPixel(x + col, y + row, 15);
            } else {
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

/* 간단한 사각형 그리기 함수 (테스트용) */
void oled_drawRect(int x, int y, int width, int height, uint8_t gray)
{
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            oled_drawPixel(x + i, y + j, gray);
        }
    }
}
// oled.c에 추가
static uint8_t frameBuffer[OLED_W/2 * OLED_H];  // 전체 화면 버퍼

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
/* 플리커 방지를 위한 추가 설정 함수 */
void OLED_setAntiFlicker(void)
{
    // 디스플레이가 켜진 후 추가 플리커 방지 설정
    OLED_write_cmd(OLED_SETCLOCKDIVIDER);
    d = 0x70;  // 더욱 느린 클록으로 설정
    OLED_write_data(&d, 1);

    // 대비를 더 낮춤
    OLED_write_cmd(OLED_SETCONTRAST);
    d = 0xCF;  // 더 낮은 대비
    OLED_write_data(&d, 1);

    printf("Anti-flicker mode enabled\r\n");
}

/* 촬영 모드 - 플리커 최소화 */
/* 촬영 모드 - 플리커 최소화 (밝기 개선 버전) */
void OLED_setCameraMode(void)
{
    // 촬영을 위한 최적 설정 (밝기 유지하면서 플리커 감소)
    OLED_write_cmd(OLED_SETCLOCKDIVIDER);
    d = 0x70;  // 느린 클록 (플리커 감소)
    OLED_write_data(&d, 1);
    OLED_write_cmd(OLED_MASTERCURRENT);
    d = 0x0F;  // 0x08에서 0x0D로 증가 (밝기 유지)
    OLED_write_data(&d, 1);

    // 대비도 최대로
       OLED_write_cmd(OLED_SETCONTRAST);
       d = 0xFF;
       OLED_write_data(&d, 1);

    HAL_Delay(200);
    printf("Camera mode enabled - balanced brightness and flicker\r\n");
}
