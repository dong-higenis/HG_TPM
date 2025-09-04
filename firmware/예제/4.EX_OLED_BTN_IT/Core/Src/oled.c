#include "oled.h"
#include <string.h>   // memset 함수 사용을 위해 필요

// 라이브러리 처럼 쓰시면 됩니다.

/* 내부에서만 사용하는 임시 변수 (static) */
static uint8_t d = 0;


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
void OLED_init(void)
{

  RST_L();
  HAL_Delay(10);
  RST_H();
  HAL_Delay(10);


  OLED_write_cmd(OLED_DISPLAYOFF);


  OLED_write_cmd(OLED_SETCOMMANDLOCK);
  d = 0x12;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_SETCLOCKDIVIDER);
  d = 0x91;
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
    uint8_t remap[2] = {0x14, 0x11};
    OLED_write_data(remap, 2);
  }


  OLED_write_cmd(OLED_SETGPIO);
  d = 0x00;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_FUNCTIONSELECT);
  d = 0x01;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_DISPLAYENHANCE_A);
  {
    uint8_t enhA[2] = {0xA0, 0xFD};
    OLED_write_data(enhA, 2);
  }

  OLED_write_cmd(OLED_SETCONTRAST);
  d = 0xFF;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_MASTERCURRENT);
  d = 0x0F;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_DEFAULTGRAYSCALE);

  OLED_write_cmd(OLED_SETPHASELENGTH);
  d = 0xE2;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_DISPLAYENHANCE_B);
  {
    uint8_t enhB[2] = {0x82, 0x20};
    OLED_write_data(enhB, 2);
  }

  OLED_write_cmd(OLED_SETPRECHARGEVOLTAGE);
  d = 0x1F;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_SETSECONDPRECHARGE);
  d = 0x08;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_SETVCOMH);
  d = 0x07;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_NORMALDISPLAY);
  OLED_write_cmd(OLED_EXITPARTIALDISPLAY);

  OLED_set_window(OLED_COL_START, OLED_COL_END, OLED_ROW_START, OLED_ROW_END);

  OLED_write_cmd(OLED_DISPLAYON);
  HAL_Delay(50);
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
