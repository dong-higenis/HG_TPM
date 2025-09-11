#ifndef AP_OLED_H_
#define AP_OLED_H_

#include "hw_oled.h"
#include "hw_sd.h"
#include "font.h"
#include <string.h>

// Application Layer 함수들
bool apOledInit(void);
void apOledClear(uint8_t gray_level);
void apOledFill(uint8_t gray_level);
void apOledUpdate(void);

// 그래픽 함수들
void apOledDrawPixel(int x, int y, uint8_t gray);
void apOledDrawChar(int x, int y, char ch, const FontDef *font, uint8_t gray);
void apOledDrawString(int x, int y, const char *str, const FontDef *font, uint8_t gray);
void apOledDrawRect(int x, int y, int width, int height, uint8_t gray);

// 비트맵 함수들
void apOledDrawBitmap(uint8_t *bmp_data, uint16_t width, uint16_t height, uint8_t x, uint8_t y);
void apOledDrawBitmapCenter(uint8_t *bmp_data, uint16_t width, uint16_t height);

// SD카드 이미지 함수
void apOledDisplayImageFromSD(char *file_name);

// 테스트 함수
void apOledTest(void);

#endif
