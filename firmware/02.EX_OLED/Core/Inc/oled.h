#include "main.h"

// oled 관련 함수는 이곳에 작성합니다.

extern SPI_HandleTypeDef hspi3; // spi를 쓸수있게, oled 파일에도 핸들러를 가져옵니다.

#define CS_L() HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#define CS_H() HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)
#define DC_CMD() HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET)
#define DC_DATA() HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET)
#define RST_L() HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)
#define RST_H() HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)

/* -------- SSD1322 명령어 모음 -------- */

// 전원 / 화면 제어
#define OLED_DISPLAYOFF           0xAE  // 화면 끄기
#define OLED_DISPLAYON            0xAF  // 화면 켜기
#define OLED_NORMALDISPLAY        0xA6  // 일반 모드 (반전 아님)
#define OLED_INVERSEDISPLAY       0xA7  // 색 반전 모드
#define OLED_EXITPARTIALDISPLAY   0xA9  // 부분 모드 해제

// 화면 주소(좌표) 설정
#define OLED_SETCOLUMNADDR        0x15  // 가로(열) 범위 설정
#define OLED_SETROWADDR           0x75  // 세로(행) 범위 설정
#define OLED_WRITERAM             0x5C  // 화면 RAM에 쓰기 시작

// 화면 배치
#define OLED_SETREMAP             0xA0  // 픽셀 데이터 좌우/상하 뒤집기, nibble 스왑
#define OLED_SETSTARTLINE         0xA1  // 화면 시작 라인
#define OLED_SETDISPLAYOFFSET     0xA2  // 화면 위/아래 밀기
#define OLED_SETMUXRATIO          0xCA  // 세로 해상도 (라인 수) 지정

// 밝기 / 전류 조절
#define OLED_SETCONTRAST          0xC1  // 밝기 (0~255)
#define OLED_MASTERCURRENT        0xC7  // 밝기 전체 비율 (0~15)

// 클럭 / 타이밍
#define OLED_SETCLOCKDIVIDER      0xB3  // 클럭 속도
#define OLED_SETPHASELENGTH       0xB1  // 신호 타이밍 (프리차지/방전)
#define OLED_SETPRECHARGEVOLTAGE  0xBB  // 프리차지 전압
#define OLED_SETSECONDPRECHARGE   0xB6  // 두 번째 프리차지 기간
#define OLED_SETVCOMH             0xBE  // 출력 전압 범위 (VCOMH)

// 기능 / 특수 설정
#define OLED_SETCOMMANDLOCK       0xFD  // 명령 잠금 해제
#define OLED_FUNCTIONSELECT       0xAB  // 내부 전원/외부 전원 선택
#define OLED_DISPLAYENHANCE_A     0xB4  // 화면 향상 A
#define OLED_DISPLAYENHANCE_B     0xD1  // 화면 향상 B
#define OLED_GRAYSCALETABLE       0xB8  // 사용자 정의 그레이스케일
#define OLED_DEFAULTGRAYSCALE     0xB9  // 기본 그레이스케일 사용

// 기타
#define OLED_SETGPIO              0xB5  // GPIO 설정
/* ---------------------------------- */

/* -------- 패널 해상도/윈도우 --------

 *   Row    : 0x00 ~ 0x3F (64라인)
 *   Column : 0x1C ~ 0x9B (128바이트 = 256픽셀)

 */
#define OLED_W                 256
#define OLED_H                 64


#define OLED_ROW_START      0x00
#define OLED_ROW_END        0x3F

#define OLED_COL_START      0x1C
#define OLED_COL_END        0x9B  /* 0x1C + 0x7F = 0x9B  (128바이트) */

static void OLED_write_cmd(uint8_t cmd);
static void OLED_write_data(const uint8_t* p, uint16_t len);
static void OLED_set_window(uint8_t col_start, uint8_t col_end,uint8_t row_start, uint8_t row_end);
void OLED_init(void);
void OLED_fill(uint8_t gray);
void OLED_test(void);

enum class_color
{
 white     = 0xFFFF,
 gray      = 0x8410,
 darkgray  = 0xAD55,
 black     = 0x0000,
 purple    = 0x8010,
 pink      = 0xFE19,
 red       = 0xF800,
 orange    = 0xFD20,
 brown     = 0xA145,
 beige     = 0xF7BB,
 yellow    = 0xFFE0,
 lightgreen= 0x9772,
 green     = 0x07E0,
 darkblue  = 0x0011,
 blue      = 0x001F,
 lightblue = 0xAEDC,
};

// 임시 저장용 변수

uint8_t d = 0;
