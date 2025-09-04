#include "main.h"

typedef struct
{
  uint8_t   width;
  uint8_t   height;
  const uint16_t *data;
} lcd_font_t;


extern lcd_font_t font_07x10;
extern lcd_font_t font_11x18;
extern lcd_font_t font_16x26;
extern lcd_font_t font_hangul;
