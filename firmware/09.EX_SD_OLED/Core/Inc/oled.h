#ifndef OLED_HD
#define OLED_HD

#include "main.h"
#include "font.h"
#include "ff.h"

/* SPI 핸들 외부 참조 */
extern SPI_HandleTypeDef hspi3;

/* LCD핀 동작 매크로 */
#define CS_L()     HAL_GPIO_WritePin(LCD_CS_GPIO_Port,  LCD_CS_Pin,  GPIO_PIN_RESET)
#define CS_H()     HAL_GPIO_WritePin(LCD_CS_GPIO_Port,  LCD_CS_Pin,  GPIO_PIN_SET)
#define DC_CMD()   HAL_GPIO_WritePin(LCD_DC_GPIO_Port,  LCD_DC_Pin,  GPIO_PIN_RESET)
#define DC_DATA()  HAL_GPIO_WritePin(LCD_DC_GPIO_Port,  LCD_DC_Pin,  GPIO_PIN_SET)
#define RST_L()    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)
#define RST_H()    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)

/* -------- SSD1322 명령어 모음집 -------- */
#define OLED_DISPLAYOFF           0xAE
#define OLED_DISPLAYON            0xAF
#define OLED_NORMALDISPLAY        0xA6
#define OLED_INVERSEDISPLAY       0xA7
#define OLED_EXITPARTIALDISPLAY   0xA9

#define OLED_SETCOLUMNADDR        0x15
#define OLED_SETROWADDR           0x75
#define OLED_WRITERAM             0x5C

#define OLED_SETREMAP             0xA0
#define OLED_SETSTARTLINE         0xA1
#define OLED_SETDISPLAYOFFSET     0xA2
#define OLED_SETMUXRATIO          0xCA

#define OLED_SETCONTRAST          0xC1
#define OLED_MASTERCURRENT        0xC7

#define OLED_SETCLOCKDIVIDER      0xB3
#define OLED_SETPHASELENGTH       0xB1
#define OLED_SETPRECHARGEVOLTAGE  0xBB
#define OLED_SETSECONDPRECHARGE   0xB6
#define OLED_SETVCOMH             0xBE

#define OLED_SETCOMMANDLOCK       0xFD
#define OLED_FUNCTIONSELECT       0xAB
#define OLED_DISPLAYENHANCE_A     0xB4
#define OLED_DISPLAYENHANCE_B     0xD1
#define OLED_GRAYSCALETABLE       0xB8
#define OLED_DEFAULTGRAYSCALE     0xB9
#define OLED_SETGPIO              0xB5

/* 해상도/윈도우 */
#define OLED_W            256
#define OLED_H            64

#define OLED_ROW_START    0x00
#define OLED_ROW_END      0x3F
#define OLED_COL_START    0x1C          /* SSD1322 256px 폭에서 좌측 바이트 시작 */
#define OLED_COL_END      0x5B          /* 0x1C + 0x7F = 0x9B (128 bytes = 256px) */

extern SPI_HandleTypeDef hspi3;
extern FIL fil;
extern FRESULT fres;
extern UINT br;

/* 외부에 노출되는 함수(헤더에 static 금지) */
void OLED_init(void);
void OLED_fill(uint8_t gray);
void oled_drawChar(int x, int y, char ch, const FontDef *font, uint8_t gray);
void oled_drawString(int x, int y, const char *str, const FontDef *font, uint8_t gray);
void OLED_test(void);

/* ======== BMP 이미지 출력 함수들 ======== */
void oled_drawPixel(int x, int y, uint8_t gray);
void oled_update(void);
void oled_drawBitmap(uint8_t *bmpData, uint16_t width, uint16_t height, uint8_t x, uint8_t y);
void oled_drawBitmapCenter(uint8_t *bmpData, uint16_t width, uint16_t height);
void oled_drawRect(int x, int y, int width, int height, uint8_t gray);
void oled_setPixelInBuffer(int x, int y, uint8_t gray);
void oled_updateDisplay(void);
void oled_clearBuffer(void);
// SD에서 BMP(32bpp) 파일을 읽어 OLED에 표시
void DisplayImageFromSD_Enhanced(char *fileName);
void OLED_setCameraMode(void);
#endif
