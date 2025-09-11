
#ifndef FONT_H
#define FONT_H

#include <stdint.h>

typedef struct FontDefTag
{
  uint8_t width;
  uint8_t height;
  const uint16_t *data;
} FontDef;

// 폰트 비트맵 배열 외부 참조 (정의는 font.c)
extern const uint16_t Font7x10[];

// 폰트 디스크립터 외부 참조 (정의는 font.c)
extern const FontDef font_07x10;

#endif // FONT_H
