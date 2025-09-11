#ifndef HW_OLED_H_
#define HW_OLED_H_

#include "def.h"

// LCD 핀 제어 매크로
#define OLED_CS_L() HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#define OLED_CS_H() HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)
#define OLED_DC_CMD() HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET)
#define OLED_DC_DATA() HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET)
#define OLED_RST_L() HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)
#define OLED_RST_H() HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)

// SSD1322 Command 정의
#define OLED_DISPLAYOFF 0xAE
#define OLED_DISPLAYON 0xAF
#define OLED_NORMALDISPLAY 0xA6
#define OLED_INVERSEDISPLAY 0xA7
#define OLED_EXITPARTIALDISPLAY 0xA9
#define OLED_SETCOLUMNADDR 0x15
#define OLED_SETROWADDR 0x75
#define OLED_WRITERAM 0x5C
#define OLED_SETREMAP 0xA0
#define OLED_SETSTARTLINE 0xA1
#define OLED_SETDISPLAYOFFSET 0xA2
#define OLED_SETMUXRATIO 0xCA
#define OLED_SETCONTRAST 0xC1
#define OLED_MASTERCURRENT 0xC7
#define OLED_SETCLOCKDIVIDER 0xB3
#define OLED_SETPHASELENGTH 0xB1
#define OLED_SETPRECHARGEVOLTAGE 0xBB
#define OLED_SETSECONDPRECHARGE 0xB6
#define OLED_SETVCOMH 0xBE
#define OLED_SETCOMMANDLOCK 0xFD
#define OLED_FUNCTIONSELECT 0xAB
#define OLED_DISPLAYENHANCE_A 0xB4
#define OLED_DISPLAYENHANCE_B 0xD1
#define OLED_GRAYSCALETABLE 0xB8
#define OLED_DEFAULTGRAYSCALE 0xB9
#define OLED_SETGPIO 0xB5
// Column/Row 범위 정의
#define OLED_ROW_START 0x00
#define OLED_ROW_END 0x3F
#define OLED_COL_START 0x1C
#define OLED_COL_END 0x5B

extern SPI_HandleTypeDef hspi3;

// Hardware Layer 함수들
bool hwOledInit(void);
void hwOledReset(void);
bool hwOledSendCmd(uint8_t cmd);
bool hwOledSendData(uint8_t *p_data, uint32_t length);
void hwOledSetWindow(uint8_t col_start, uint8_t col_end, uint8_t row_start, uint8_t row_end);

#endif
