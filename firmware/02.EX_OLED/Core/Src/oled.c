#include "oled.h"

/*
 * SSD1322 OLED 동작 원리 요약
 * ----------------------------
 * - SSD1322는 "명령"과 "데이터"를 구분해서 받는다.
 *   DC = LOW → 명령 모드
 *   DC = HIGH → 데이터 모드
 *
 * - 화면을 켜려면:
 *   "Display OFF" → 여러 설정(클럭, 오프셋, 밝기 등) → "Display ON"
 *
 * - 픽셀 데이터 포맷:
 *   4비트(0~15 단계 그레이스케일)
 *   1바이트 = 픽셀 2개 (상위 4비트 + 하위 4비트)
 *
 * - 해상도:
 *   이 모듈은 256 x 64 픽셀
 */

 // 초기화용 변수
static uint8_t temp = 0;

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
  OLED_write_data(&d, 1);

  // 3. 클럭 설정 (화면 동작 속도 관련)
  OLED_write_cmd(OLED_SETCLOCKDIVIDER);
  temp = 0x91; // 분주비=1, 오실레이터=9
  OLED_write_data(&d, 1);

  // 4. 세로 해상도 (라인 수 = 64)
  OLED_write_cmd(OLED_SETMUXRATIO);
  temp = 0x3F; // 63+1 = 64라인
  OLED_write_data(&d, 1);

  // 5. 화면 세로 오프셋 (위/아래로 밀기)
  OLED_write_cmd(OLED_SETDISPLAYOFFSET);
  temp = 0x00; // 안 밀기
  OLED_write_data(&d, 1);

  // 6. 시작 라인 (RAM에서 0번 라인부터 출력)
  OLED_write_cmd(OLED_SETSTARTLINE);
  temp = 0x00;
  OLED_write_data(&d, 1);

  // 7. 화면 배치 (좌우/상하 반전, nibble 스왑 등)
  OLED_write_cmd(OLED_SETREMAP);
  uint8_t remap[2] = {0x14, 0x11}; // 보편적인 설정
  OLED_write_data(remap, 2);

  // 8. GPIO 비활성화 (안 씀)
  OLED_write_cmd(OLED_SETGPIO);
  temp = 0x00;
  OLED_write_data(&d, 1);

  // 9. 내부 전원(VDD) 사용
  OLED_write_cmd(OLED_FUNCTIONSELECT);
  temp = 0x01;
  OLED_write_data(&d, 1);

  // 10. 화면 향상 A (화질 최적화)
  OLED_write_cmd(OLED_DISPLAYENHANCE_A);
  uint8_t enhA[2] = {0xA0, 0xFD};
  OLED_write_data(enhA, 2);

  // 11. 밝기 (세그먼트 전류, 0~255)
  OLED_write_cmd(OLED_SETCONTRAST);
  temp = 0xFF; // 최대 밝기
  OLED_write_data(&d, 1);

  // 12. 밝기 전체 스케일 (0~15)
  OLED_write_cmd(OLED_MASTERCURRENT);
  temp = 0x0F; // 최대
  OLED_write_data(&d, 1);

  // 13. 기본 그레이스케일 테이블 사용
  OLED_write_cmd(OLED_DEFAULTGRAYSCALE);

  // 14. 신호 타이밍 (화면 깜빡임/잔상 관련)
  OLED_write_cmd(OLED_SETPHASELENGTH);
  temp = 0xE2;
  OLED_write_data(&d, 1);

  // 15. 화면 향상 B
  OLED_write_cmd(OLED_DISPLAYENHANCE_B);
  uint8_t enhB[2] = {0x82, 0x20};
  OLED_write_data(enhB, 2);

  // 16. 프리차지 전압
  OLED_write_cmd(OLED_SETPRECHARGEVOLTAGE);
  temp = 0x1F;
  OLED_write_data(&d, 1);

  // 17. 두 번째 프리차지 기간
  OLED_write_cmd(OLED_SETSECONDPRECHARGE);
  temp = 0x08;
  OLED_write_data(&d, 1);

  // 18. 출력 전압 범위 (VCOMH)
  OLED_write_cmd(OLED_SETVCOMH);
  temp = 0x07;
  OLED_write_data(&d, 1);

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

