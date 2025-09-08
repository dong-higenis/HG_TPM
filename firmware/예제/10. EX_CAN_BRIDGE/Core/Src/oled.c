#include "oled.h"
#include <string.h>   // memset 함수 사용을 위해 필요

// 라이브러리 처럼 쓰시면 됩니다.

/* 내부에서만 사용하는 임시 변수 (static) */
static uint8_t d = 0;

/* ======== Static 함수들 ======== */

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

/* ======== Public 함수들 ======== */

/* OLED 디스플레이를 초기화하는 함수 */
void OLED_init(void)
{
  // 1단계: 하드웨어 리셋
  RST_L();
  HAL_Delay(10);    // RESET핀을 LOW로 10ms
  RST_H();
  HAL_Delay(10);    // RESET핀을 HIGH로 10ms

  // 2단계: 디스플레이 끄기
  OLED_write_cmd(OLED_DISPLAYOFF);

  // 3단계: 기본 설정들
  OLED_write_cmd(OLED_SETCOMMANDLOCK);  // 명령어 잠금 해제
  d = 0x12;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_SETCLOCKDIVIDER); // 클럭 주파수 설정
  d = 0x91;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_SETMUXRATIO);     // 화면 높이 설정 (64줄)
  d = 0x3F;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_SETDISPLAYOFFSET); // 화면 위치 오프셋
  d = 0x00;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_SETSTARTLINE);     // 시작 줄 설정
  d = 0x00;
  OLED_write_data(&d, 1);

  // 4단계: 화면 방향과 색상 배치 설정
  OLED_write_cmd(OLED_SETREMAP);
  {
    uint8_t remap[2] = {0x6, 0x11};  // 니블 순서, 듀얼COM 설정
    OLED_write_data(remap, 2);
  } // Dual COM 모드로 설정시, Column 범위가 [ 0x1C <= x <= 0x5B ]가 됩니다.



  // 5단계: GPIO와 기능 설정 ( 안씀 )
  OLED_write_cmd(OLED_SETGPIO);
  d = 0x00;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_FUNCTIONSELECT);  // 내부 전원 사용
  d = 0x01;
  OLED_write_data(&d, 1);

  // 6단계: 화면 품질 향상 설정
  OLED_write_cmd(OLED_DISPLAYENHANCE_A);
  {
    uint8_t enhA[2] = {0xA0, 0xFD};
    OLED_write_data(enhA, 2);
  }

  // 7단계: 밝기와 전류 설정
  OLED_write_cmd(OLED_SETCONTRAST);     // 최대 밝기
  d = 0xFF;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_MASTERCURRENT);   // 마스터 전류
  d = 0x0F;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_DEFAULTGRAYSCALE); // 기본 그레이스케일 사용

  // 8단계: 전기적 특성 설정
  OLED_write_cmd(OLED_SETPHASELENGTH);  // 페이즈 길이
  d = 0xE2;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_DISPLAYENHANCE_B); // 화면 품질 향상 B
  {
    uint8_t enhB[2] = {0x82, 0x20};
    OLED_write_data(enhB, 2);
  }

  OLED_write_cmd(OLED_SETPRECHARGEVOLTAGE); // 프리차지 전압
  d = 0x1F;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_SETSECONDPRECHARGE);  // 두 번째 프리차지
  d = 0x08;
  OLED_write_data(&d, 1);

  OLED_write_cmd(OLED_SETVCOMH);            // VCOM 전압
  d = 0x07;
  OLED_write_data(&d, 1);

  // 9단계: 디스플레이 모드 설정
  OLED_write_cmd(OLED_NORMALDISPLAY);       // 정상 디스플레이 모드
  OLED_write_cmd(OLED_EXITPARTIALDISPLAY);  // 부분 디스플레이 모드 해제

  // 10단계: 전체 화면을 그리기 영역으로 설정
  OLED_set_window(OLED_COL_START, OLED_COL_END, OLED_ROW_START, OLED_ROW_END);

  // 11단계: 디스플레이 켜기
  OLED_write_cmd(OLED_DISPLAYON);
  HAL_Delay(50);  // 안정화 대기
}

/* 화면 전체를 하나의 색으로 채우는 함수 */
void OLED_fill(uint8_t gray)  // gray: 0(검정)~15(흰색)
{
  // 4bpp에서는 1바이트에 2픽셀이 들어감 (4bit씩)
  uint8_t b = (gray << 4) | (gray & 0x0F);  // 왼쪽4bit + 오른쪽4bit
  uint8_t line[OLED_W/2];  // 한 줄에 필요한 바이트 수 (256픽셀 / 2 = 128바이트)
  memset(line, b, sizeof(line));  // 배열을 같은 값으로 채움

  // 전체 화면을 그리기 영역으로 설정
  OLED_set_window(OLED_COL_START, OLED_COL_END, OLED_ROW_START, OLED_ROW_END);
  OLED_write_cmd(OLED_WRITERAM);  // 메모리 쓰기 시작

  // 64줄을 반복해서 같은 데이터 전송
  for (int y = 0; y < OLED_H; y++) {
    OLED_write_data(line, sizeof(line));
  }
}

/* ======== 좌표 변환 함수들 ======== */

/* 픽셀의 x좌표를 OLED의 column 바이트 주소로 변환 */
static uint8_t colbyte_from_x(int x)
{
  // 4bpp에서는 2픽셀당 1바이트이므로 x를 2로 나눔
  return OLED_COL_START + (x >> 1);  // x >> 1은 x / 2와 같음
}

/* 픽셀의 y좌표를 OLED의 row 주소로 변환 */
static uint8_t rowaddr_from_y(int y)
{
  return OLED_ROW_START + y;  // y좌표는 그대로 사용
}

/* ======== 문자 그리기 함수들 ======== */

/* 하나의 문자를 화면에 그리는 함수 */
void oled_drawChar(int x, int y, char ch, const FontDef *font, uint8_t gray)
{
  // 1단계: 입력값 검증
  if (ch < 32 || ch > 126) return;  // 출력 가능한 ASCII 문자만 허용
  if (gray > 15) gray = 15;         // 최대 밝기 제한

  // 2단계: 화면 범위 검사
  if (x < 0 || (x + font->width) > OLED_W)   return;  // 가로 범위 초과
  if (y < 0 || (y + font->height) > OLED_H)  return;  // 세로 범위 초과

  // 3단계: 폰트 데이터에서 문자 찾기
  const int stride = font->height;           // 한 문자의 데이터 줄 수
  const int start  = (ch - 32) * stride;     // 문자 데이터 시작 위치 ('A'는 65-32=33번째)
  const int bytes_per_row = (font->width + 1) / 2;  // 한 줄당 필요한 바이트 수

  uint8_t linebuf[OLED_W/2];  // 한 줄 데이터를 저장할 버퍼

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
      int byte_idx = (col >> 1);              // 현재 픽셀이 들어갈 바이트 위치
      int left_nibble = ((col & 1) == 0);     // 바이트의 왼쪽 4bit인지 오른쪽 4bit인지

      // 현재 픽셀이 켜져있는지 확인
      if (mask & (0x8000 >> col))  // 0x8000에서 오른쪽으로 col번 이동한 비트 확인
      {
        // 픽셀이 켜져있으면 해당 위치에 색상 설정
        if (left_nibble)
          linebuf[byte_idx] |= (gray << 4);   // 왼쪽 4bit에 색상
        else
          linebuf[byte_idx] |= (gray & 0x0F); // 오른쪽 4bit에 색상
      }
    }

    // 6단계: 현재 줄을 OLED에 전송
    uint8_t col_start = colbyte_from_x(x);           // 시작 column 주소
    uint8_t col_end   = col_start + bytes_per_row - 1;  // 끝 column 주소
    uint8_t row_addr  = rowaddr_from_y(y + row);     // 현재 row 주소

    OLED_set_window(col_start, col_end, row_addr, row_addr);  // 그리기 영역 설정
    OLED_write_cmd(OLED_WRITERAM);                   // 메모리 쓰기 시작
    OLED_write_data(linebuf, bytes_per_row);         // 데이터 전송
  }
}

/* 문자열을 화면에 그리는 함수 */
void oled_drawString(int x, int y, const char *str, const FontDef *font, uint8_t gray)
{
  int cx = x, cy = y;  // 현재 커서 위치

  // 문자열의 끝까지 반복
  while (*str) {
    if (*str == '\n') {  // 줄바꿈 문자
      cy += font->height + 1;  // 다음 줄로 이동 (+1은 줄 간격)
      cx = x;                  // 맨 왼쪽으로 돌아가기
     // if (cx & 1) cx--;        // 시작 위치를 짝수로 맞춤 (니블 정렬)
    } // 사용시 "str\nstr" 이런식으로 쓰시면 됩니다.

    else if (*str != '\r') {   // 캐리지 리턴이 아닌 일반 문자
      oled_drawChar(cx, cy, *str, font, gray);  // 현재 위치에 문자 그리기
      cx += 4;  // 다음 문자 위치로 이동 (4픽셀 간격으로 설정)

      // 원래 코드: cx += font->width + 1; (폰트 너비 + 1픽셀 간격)
      // 원래 코드: if (cx & 1) cx++;      (다음 위치를 짝수로 맞춤)
      // → 이 두 줄이 문자 사이에 큰 간격을 만들었음
    }
    str++;  // 다음 문자로 이동
  }
}
