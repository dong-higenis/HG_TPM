/*
 * lcd.c
 *
 *  Created on: 2020. 12. 27.
 *      Author: baram
 */


#include "lcd.h"
#include "cli.h"


#ifdef _USE_HW_LCD
#include "gpio.h"
#include "hangul/han.h"
#include "lcd/lcd_fonts.h"

#ifdef _USE_HW_SSD1322
#include "lcd/ssd1322.h"
#endif

#ifdef _USE_HW_PWM
#include "pwm.h"
#endif

#ifdef _USE_HW_ADC
#include "adc.h"
#endif

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) \
  {                         \
    int16_t t = a;          \
    a         = b;          \
    b         = t;          \
  }
#endif


#define LCD_FONT_RESIZE_WIDTH 64

#define MAKECOL(r, g, b)      (((r) << 11) | ((g) << 5) | (b))


#define LCD_OPT_DEF           __attribute__((optimize("O2")))
#define _PIN_DEF_BL_CTL       1

typedef struct
{
  int16_t x;
  int16_t y;
} lcd_pixel_t;

static lcd_driver_t lcd;


static bool          is_init         = false;
static volatile bool is_tx_done      = true;
static uint8_t       backlight_value = 100;
static uint8_t       frame_index     = 0;
static LcdFont       lcd_font        = LCD_FONT_HAN;

static bool lcd_request_draw = false;

static volatile uint32_t fps_pre_time;
static volatile uint32_t fps_time;
static volatile uint32_t fps_count = 0;


static volatile int32_t  draw_fps        = 20;
static volatile uint32_t draw_frame_time = 0;
static volatile uint32_t draw_pre_time   = 0;

#ifdef _USE_HW_SSD1322
static uint8_t *p_draw_frame_buf = NULL;
static uint8_t __attribute__((aligned(64))) * frame_buffer[1];
#else
static uint16_t                             *p_draw_frame_buf = NULL;
static uint16_t __attribute__((aligned(64))) frame_buffer[1][HW_LCD_WIDTH * HW_LCD_HEIGHT];
#endif

static lcd_font_t *font_tbl[LCD_FONT_MAX] = {&font_07x10, &font_11x18, &font_16x26, &font_hangul};

static volatile bool requested_from_thread = false;


static void disHanFont(int x, int y, han_font_t *FontPtr, uint16_t textcolor);
static void disEngFont(int x, int y, char ch, lcd_font_t *font, uint16_t textcolor);
static void lcdDrawLineBuffer(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color, lcd_pixel_t *line);

#ifdef _USE_HW_CLI
static void cliLcd(cli_args_t *args);
#endif

void TransferDoneISR(void)
{
  fps_time     = millis() - fps_pre_time;
  fps_pre_time = millis();

  if (fps_time > 0)
  {
    fps_count = 1000 / fps_time;
  }

  lcd_request_draw = false;
}

bool lcdInit(void)
{
  backlight_value = 100;

  is_init = ssd1322Init();
  ssd1322InitDriver(&lcd);

  frame_buffer[0]  = ssd1322GetBuffer();
  p_draw_frame_buf = frame_buffer[0];

  lcd.setCallBack(TransferDoneISR);

  draw_pre_time = millis();

  memset(frame_buffer[0], 0x00, LCD_WIDTH * LCD_HEIGHT / 8);
  // memset(frame_buffer, 0x00, sizeof(frame_buffer));

  p_draw_frame_buf = frame_buffer[frame_index];


  lcdDrawFillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, black);
  lcdUpdateDraw();

  if (is_init != true)
  {
    return false;
  }

#ifdef _USE_HW_CLI
  cliAdd("lcd", cliLcd);
#endif

  return true;
}

uint32_t lcdGetDrawTime(void)
{
  return draw_frame_time;
}

bool lcdIsInit(void)
{
  return is_init;
}

void lcdReset(void)
{
  lcd.reset();
}

uint8_t lcdGetBackLight(void)
{
  return backlight_value;
}

void lcdSetBackLight(uint8_t value)
{
  value = constrain(value, 0, 100);

  if (value != backlight_value)
  {
    backlight_value = value;
  }

#ifdef _USE_HW_PWM
  pwmWrite(0, map(value, 0, 100, 0, 255));
#else
  if (backlight_value > 0)
  {
    // gpioPinWrite(_PIN_DEF_BL_CTL, _DEF_HIGH);
  }
  else
  {
    // gpioPinWrite(_PIN_DEF_BL_CTL, _DEF_LOW);
  }
#endif
}

LCD_OPT_DEF uint32_t lcdReadPixel(uint16_t x_pos, uint16_t y_pos)
{
  return p_draw_frame_buf[y_pos * LCD_WIDTH + x_pos];
}

#if 1
LCD_OPT_DEF void lcdDrawPixel(int16_t x_pos, int16_t y_pos, uint32_t rgb_code)
{
  // 범위를 벗어나면 아무 작업도 하지 않음
  if (x_pos < 0 || x_pos >= LCD_WIDTH || y_pos < 0 || y_pos >= LCD_HEIGHT)
    return;

  // 1D 인덱스 계산 (1비트당 1픽셀, 1바이트당 8픽셀)
  uint16_t byte_index = (y_pos * LCD_WIDTH + x_pos) / 8;
  uint8_t  bit_index  = 7 - (x_pos % 8); // MSB부터 사용

  // 픽셀 설정
  if (rgb_code != black)
  {
    // 흰색 (픽셀 ON)
    p_draw_frame_buf[byte_index] |= (1 << bit_index);
  }
  else
  {
    // 검정색 (픽셀 OFF)
    p_draw_frame_buf[byte_index] &= ~(1 << bit_index);
  }
}
#else
LCD_OPT_DEF void lcdDrawPixel(int16_t x_pos, int16_t y_pos, uint32_t rgb_code)
{
  return;
  // uint8_t brightness = 0xFF;

  // if (x > (LCD_WIDTH - 1) || y > (LCD_HEIGHT - 1)) return;

  // if ((y * LCD_WIDTH + x) % 2 == 1)
  // {
  //   p_draw_frame_buf[((y * LCD_WIDTH) + x) / 2] = (p_draw_frame_buf[((y * LCD_WIDTH) + x) / 2] & 0xF0) | brightness;
  // }
  // else
  // {
  //   p_draw_frame_buf[((y * LCD_WIDTH) + x) / 2] = (p_draw_frame_buf[((y * LCD_WIDTH) + x) / 2] & 0x0F) | (brightness << 4);
  // }
  if (x_pos < 0 || x_pos >= LCD_WIDTH) return;
  if (y_pos < 0 || y_pos >= LCD_HEIGHT) return;

#ifdef _USE_HW_SSD1322
  // if (rgb_code > 0)
  // {
  //   p_draw_frame_buf[(x_pos) + ((y_pos >> 3) * LCD_WIDTH)] |= 1 << (7 - (y_pos % 8));    // 1 << (y_pos % 8);
  // }
  // else
  // {
  //   p_draw_frame_buf[(x_pos) + ((y_pos >> 3) * LCD_WIDTH)] &= ~(1 << (7 - (y_pos % 8))); // 1 << (y_pos % 8);
  // }
  if ((x_pos % 2) == 1) // 홀수 픽셀일 경우 (오른쪽 4비트)
  {
    p_draw_frame_buf[(y_pos * LCD_WIDTH + x_pos) / 2] = (p_draw_frame_buf[(y_pos * LCD_WIDTH + x_pos) / 2] & 0xF0) | (rgb_code & 0x0F);
  }
  else                  // 짝수 픽셀일 경우 (왼쪽 4비트)
  {
    p_draw_frame_buf[(y_pos * LCD_WIDTH + x_pos) / 2] = (p_draw_frame_buf[(y_pos * LCD_WIDTH + x_pos) / 2] & 0x0F) | ((rgb_code & 0x0F) << 4);
  }

#else
  p_draw_frame_buf[y_pos * LCD_WIDTH + x_pos] = rgb_code;
#endif
}
#endif
// LCD_OPT_DEF void lcdDrawPixel(int16_t x_pos, int16_t y_pos, uint32_t rgb_code)
//{
//   if (x_pos < 0 || x_pos >= LCD_WIDTH) return;
//   if (y_pos < 0 || y_pos >= LCD_HEIGHT) return;
//
//   // 1비트 모노크롬 방식으로 버퍼에 저장
//   if (rgb_code > 0)
//   {
//     ssd1322_buffer[x_pos + (y_pos / 8) * LCD_WIDTH] |= 1 << (y_pos % 8);
//   }
//   else
//   {
//     ssd1322_buffer[x_pos + (y_pos / 8) * LCD_WIDTH] &= ~(1 << (y_pos % 8));
//   }
// }

LCD_OPT_DEF void lcdClear(uint32_t rgb_code)
{
  lcdClearBuffer(rgb_code);

  lcdUpdateDraw();
}

LCD_OPT_DEF void lcdClearBuffer(uint32_t rgb_code)
{
#ifdef _USE_HW_SSD1322
  uint8_t *p_buf = (uint8_t *)lcdGetFrameBuffer();

  if (rgb_code > 0)
  {
    memset(p_buf, 0xFF, LCD_WIDTH * LCD_HEIGHT / 8);
  }
  else
  {
    memset(p_buf, 0x00, LCD_WIDTH * LCD_HEIGHT / 8);
  }
#else
  uint16_t *p_buf = lcdGetFrameBuffer();

  for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++)
  {
    p_buf[i] = rgb_code;
  }
#endif
}

LCD_OPT_DEF void lcdDrawFillCircle(int32_t x0, int32_t y0, int32_t r, uint16_t color)
{
  int32_t x  = 0;
  int32_t dx = 1;
  int32_t dy = r + r;
  int32_t p  = -(r >> 1);


  lcdDrawHLine(x0 - r, y0, dy + 1, color);

  while (x < r)
  {
    if (p >= 0)
    {
      dy -= 2;
      p  -= dy;
      r--;
    }

    dx += 2;
    p  += dx;

    x++;

    lcdDrawHLine(x0 - r, y0 + x, 2 * r + 1, color);
    lcdDrawHLine(x0 - r, y0 - x, 2 * r + 1, color);
    lcdDrawHLine(x0 - x, y0 + r, 2 * x + 1, color);
    lcdDrawHLine(x0 - x, y0 - r, 2 * x + 1, color);
  }
}

LCD_OPT_DEF void lcdDrawCircleHelper(int32_t x0, int32_t y0, int32_t r, uint8_t cornername, uint32_t color)
{
  int32_t f     = 1 - r;
  int32_t ddF_x = 1;
  int32_t ddF_y = -2 * r;
  int32_t x     = 0;

  while (x < r)
  {
    if (f >= 0)
    {
      r--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    if (cornername & 0x4)
    {
      lcdDrawPixel(x0 + x, y0 + r, color);
      lcdDrawPixel(x0 + r, y0 + x, color);
    }
    if (cornername & 0x2)
    {
      lcdDrawPixel(x0 + x, y0 - r, color);
      lcdDrawPixel(x0 + r, y0 - x, color);
    }
    if (cornername & 0x8)
    {
      lcdDrawPixel(x0 - r, y0 + x, color);
      lcdDrawPixel(x0 - x, y0 + r, color);
    }
    if (cornername & 0x1)
    {
      lcdDrawPixel(x0 - r, y0 - x, color);
      lcdDrawPixel(x0 - x, y0 - r, color);
    }
  }
}

LCD_OPT_DEF void lcdDrawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color)
{
  // smarter version
  lcdDrawHLine(x + r, y, w - r - r, color);         // Top
  lcdDrawHLine(x + r, y + h - 1, w - r - r, color); // Bottom
  lcdDrawVLine(x, y + r, h - r - r, color);         // Left
  lcdDrawVLine(x + w - 1, y + r, h - r - r, color); // Right

  // draw four corners
  lcdDrawCircleHelper(x + r, y + r, r, 1, color);
  lcdDrawCircleHelper(x + w - r - 1, y + r, r, 2, color);
  lcdDrawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
  lcdDrawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

LCD_OPT_DEF void lcdDrawFillCircleHelper(int32_t x0, int32_t y0, int32_t r, uint8_t cornername, int32_t delta, uint32_t color)
{
  int32_t f     = 1 - r;
  int32_t ddF_x = 1;
  int32_t ddF_y = -r - r;
  int32_t y     = 0;

  delta++;

  while (y < r)
  {
    if (f >= 0)
    {
      r--;
      ddF_y += 2;
      f     += ddF_y;
    }

    y++;
    ddF_x += 2;
    f     += ddF_x;

    if (cornername & 0x1)
    {
      lcdDrawHLine(x0 - r, y0 + y, r + r + delta, color);
      lcdDrawHLine(x0 - y, y0 + r, y + y + delta, color);
    }
    if (cornername & 0x2)
    {
      lcdDrawHLine(x0 - r, y0 - y, r + r + delta, color); // 11995, 1090
      lcdDrawHLine(x0 - y, y0 - r, y + y + delta, color);
    }
  }
}

LCD_OPT_DEF void lcdDrawFillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color)
{
  // smarter version
  lcdDrawFillRect(x, y + r, w, h - r - r, color);

  // draw four corners
  lcdDrawFillCircleHelper(x + r, y + h - r - 1, r, 1, w - r - r - 1, color);
  lcdDrawFillCircleHelper(x + r, y + r, r, 2, w - r - r - 1, color);
}

LCD_OPT_DEF void lcdDrawTriangle(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, uint32_t color)
{
  lcdDrawLine(x1, y1, x2, y2, color);
  lcdDrawLine(x1, y1, x3, y3, color);
  lcdDrawLine(x2, y2, x3, y3, color);
}

void lcdDrawFillTriangle(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, uint32_t color)
{
  uint16_t max_line_size_12 = cmax(abs(x1 - x2), abs(y1 - y2));
  uint16_t max_line_size_13 = cmax(abs(x1 - x3), abs(y1 - y3));
  uint16_t max_line_size_23 = cmax(abs(x2 - x3), abs(y2 - y3));
  uint16_t max_line_size    = max_line_size_12;
  uint16_t i                = 0;

  if (max_line_size_13 > max_line_size)
  {
    max_line_size = max_line_size_13;
  }
  if (max_line_size_23 > max_line_size)
  {
    max_line_size = max_line_size_23;
  }

  lcd_pixel_t line[max_line_size];

  lcdDrawLineBuffer(x1, y1, x2, y2, color, line);
  for (i = 0; i < max_line_size_12; i++)
  {
    lcdDrawLine(x3, y3, line[i].x, line[i].y, color);
  }
  lcdDrawLineBuffer(x1, y1, x3, y3, color, line);
  for (i = 0; i < max_line_size_13; i++)
  {
    lcdDrawLine(x2, y2, line[i].x, line[i].y, color);
  }
  lcdDrawLineBuffer(x2, y2, x3, y3, color, line);
  for (i = 0; i < max_line_size_23; i++)
  {
    lcdDrawLine(x1, y1, line[i].x, line[i].y, color);
  }
}

void lcdSetFps(int32_t fps)
{
  draw_fps = fps;
}

uint32_t lcdGetFps(void)
{
  return fps_count;
}

uint32_t lcdGetFpsTime(void)
{
  return fps_time;
}

bool lcdDrawAvailable(void)
{
  bool ret = false;

  ret = !lcd_request_draw;

  return ret;
}

bool lcdRequestDraw(void)
{
  if (is_init != true)
  {
    return false;
  }
  if (lcd_request_draw == true)
  {
    return false;
  }

  lcd.setWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

  lcd_request_draw = true;
  lcd.sendBuffer((uint8_t *)frame_buffer[frame_index], LCD_WIDTH * LCD_HEIGHT, 0);

  return true;
}

void lcdUpdateDraw(void)
{
  uint32_t pre_time;

  if (is_init != true)
  {
    return;
  }

  lcdRequestDraw();

  pre_time = millis();
  while (lcdDrawAvailable() != true)
  {
    delay(1);
    if (millis() - pre_time >= 100)
    {
      break;
    }
  }
}

void lcdSetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  if (is_init != true)
  {
    return;
  }

  lcd.setWindow(x, y, w, h);
}

uint16_t *lcdGetFrameBuffer(void)
{
  return (uint16_t *)p_draw_frame_buf;
}

uint16_t *lcdGetCurrentFrameBuffer(void)
{
  return (uint16_t *)frame_buffer[frame_index];
}

void lcdDisplayOff(void)
{
}

void lcdDisplayOn(void)
{
  lcdSetBackLight(lcdGetBackLight());
}

int32_t lcdGetWidth(void)
{
  return LCD_WIDTH;
}

int32_t lcdGetHeight(void)
{
  return LCD_HEIGHT;
}

void lcdDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);

  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;


  if (steep)
  {
    _swap_int16_t(x0, y0);
    _swap_int16_t(x1, y1);
  }

  if (x0 > x1)
  {
    _swap_int16_t(x0, x1);
    _swap_int16_t(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1)
  {
    ystep = 1;
  }
  else
  {
    ystep = -1;
  }

  for (; x0 <= x1; x0++)
  {
    if (steep)
    {
      lcdDrawPixel(y0, x0, color);
    }
    else
    {
      lcdDrawPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0)
    {
      y0  += ystep;
      err += dx;
    }
  }
}

void lcdDrawLineBuffer(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color, lcd_pixel_t *line)
{
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);

  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;


  if (steep)
  {
    _swap_int16_t(x0, y0);
    _swap_int16_t(x1, y1);
  }

  if (x0 > x1)
  {
    _swap_int16_t(x0, x1);
    _swap_int16_t(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1)
  {
    ystep = 1;
  }
  else
  {
    ystep = -1;
  }

  for (; x0 <= x1; x0++)
  {
    if (steep)
    {
      if (line != NULL)
      {
        line->x = y0;
        line->y = x0;
      }
      lcdDrawPixel(y0, x0, color);
    }
    else
    {
      if (line != NULL)
      {
        line->x = x0;
        line->y = y0;
      }
      lcdDrawPixel(x0, y0, color);
    }
    if (line != NULL)
    {
      line++;
    }
    err -= dy;
    if (err < 0)
    {
      y0  += ystep;
      err += dx;
    }
  }
}

void lcdDrawVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
  lcdDrawLine(x, y, x, y + h - 1, color);
}

void lcdDrawHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
  lcdDrawLine(x, y, x + w - 1, y, color);
}

void lcdDrawFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  for (int16_t i = x; i < x + w; i++)
  {
    lcdDrawVLine(i, y, h, color);
  }
}

void lcdDrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  lcdDrawHLine(x, y, w, color);
  lcdDrawHLine(x, y + h - 1, w, color);
  lcdDrawVLine(x, y, h, color);
  lcdDrawVLine(x + w - 1, y, h, color);
}

void lcdDrawFillScreen(uint16_t color)
{
  lcdDrawFillRect(0, 0, HW_LCD_WIDTH, HW_LCD_HEIGHT, color);
}
#if 0
void lcdPrintf(int x, int y, uint16_t color, const char *fmt, ...)
{
  va_list arg;
  va_start(arg, fmt);
  int32_t    len;
  char       print_buffer[256];
  int        Size_Char;
  int        i, x_Pre = x;
  han_font_t FontBuf;
  uint8_t    font_width;
  uint8_t    font_height;


  len = vsnprintf(print_buffer, 255, fmt, arg);
  va_end(arg);

  if (font_tbl[lcd_font]->data != NULL)
  {
    for (i = 0; i < len; i += Size_Char)
    {
      disEngFont(x, y, print_buffer[i], font_tbl[lcd_font], color);

      Size_Char    = 1;
      font_width   = font_tbl[lcd_font]->width;
      font_height  = font_tbl[lcd_font]->height;
      x           += font_width;

      if ((x + font_width) > HW_LCD_WIDTH)
      {
        x  = x_Pre;
        y += font_height;
      }
    }
  }
  else
  {
    for (i = 0; i < len; i += Size_Char)
    {
      hanFontLoad(&print_buffer[i], &FontBuf);

      disHanFont(x, y, &FontBuf, color);

      Size_Char = FontBuf.Size_Char;
      if (Size_Char >= 2)
      {
        font_width  = 16;
        x          += 2 * 8;
      }
      else
      {
        font_width  = 8;
        x          += 1 * 8;
      }

      if ((x + font_width) > HW_LCD_WIDTH)
      {
        x  = x_Pre;
        y += 16;
      }

      if (FontBuf.Code_Type == PHAN_END_CODE) break;
    }
  }
}
#else
void lcdPrintf(int x, int y, uint16_t color, const char *fmt, ...)
{
  va_list arg;
  va_start(arg, fmt);
  int32_t    len;
  char       print_buffer[256];
  int        Size_Char;
  int        i, x_Pre = x;
  han_font_t FontBuf;
  uint8_t    font_width;
  uint8_t    font_height;
  const uint8_t scale = 2;  // 스케일 값

  len = vsnprintf(print_buffer, 255, fmt, arg);
  va_end(arg);

  if (font_tbl[lcd_font]->data != NULL)
  {
    for (i = 0; i < len; i += Size_Char)
    {
      disEngFont(x, y, print_buffer[i], font_tbl[lcd_font], color);

      Size_Char    = 1;
      font_width   = font_tbl[lcd_font]->width;
      font_height  = font_tbl[lcd_font]->height;
      x           += font_width;

      if ((x + font_width) > HW_LCD_WIDTH)
      {
        x  = x_Pre;
        y += font_height;
      }
    }
  }
  else // 한글 폰트
  {
    for (i = 0; i < len; i += Size_Char)
    {
      hanFontLoad(&print_buffer[i], &FontBuf);

      disHanFont(x, y, &FontBuf, color);

      Size_Char = FontBuf.Size_Char;
      if (Size_Char >= 2)
      {
        font_width  = 16 * scale;  // 스케일 적용
        x          += 16 * scale;  // x 위치도 스케일 적용
      }
      else
      {
        font_width  = 8 * scale;   // 영문자 폭도 스케일 적용
        x          += 8 * scale;
      }

      if ((x + font_width) > HW_LCD_WIDTH)
      {
        x  = x_Pre;
        y += 16 * scale;  // y축 간격도 스케일 적용
      }

      if (FontBuf.Code_Type == PHAN_END_CODE) break;
    }
  }
}
#endif

uint32_t lcdGetStrWidth(const char *fmt, ...)
{
  va_list arg;
  va_start(arg, fmt);
  int32_t    len;
  char       print_buffer[256];
  int        Size_Char;
  int        i;
  han_font_t FontBuf;
  uint32_t   str_len;


  len = vsnprintf(print_buffer, 255, fmt, arg);
  va_end(arg);

  str_len = 0;

  for (i = 0; i < len; i += Size_Char)
  {
    hanFontLoad(&print_buffer[i], &FontBuf);

    Size_Char = FontBuf.Size_Char;

    str_len += (Size_Char * 8);

    if (FontBuf.Code_Type == PHAN_END_CODE) break;
  }

  return str_len;
}

#if 0
void disHanFont(int x, int y, han_font_t *FontPtr, uint16_t textcolor)
{
  uint16_t i, j, Loop;
  uint16_t FontSize = FontPtr->Size_Char;
  uint16_t index_x;

  if (FontSize > 2)
  {
    FontSize = 2;
  }

  for (i = 0; i < 16; i++)         // 16 Lines per Font/Char
  {
    index_x = 0;
    for (j = 0; j < FontSize; j++) // 16 x 16 (2 Bytes)
    {
      uint8_t font_data;

      font_data = FontPtr->FontBuffer[i * FontSize + j];

      for (Loop = 0; Loop < 8; Loop++)
      {
        if ((font_data << Loop) & (0x80))
        {
          lcdDrawPixel(x + index_x, y + i, textcolor);
        }
        index_x++;
      }
    }
  }
}
#else
// 기존 disHanFont 함수를 스케일링이 가능하도록 수정
static void disHanFont(int x, int y, han_font_t *FontPtr, uint16_t textcolor)
{
  uint16_t i, j, Loop;
  uint16_t FontSize = FontPtr->Size_Char;
  uint16_t index_x;
  const uint8_t scale = 2;  // 고정 스케일 값 (또는 전역 변수로 설정)

  if (FontSize > 2)
  {
    FontSize = 2;
  }

  for (i = 0; i < 16; i++)
  {
    index_x = 0;
    for (j = 0; j < FontSize; j++)
    {
      uint8_t font_data;
      font_data = FontPtr->FontBuffer[i * FontSize + j];

      for (Loop = 0; Loop < 8; Loop++)
      {
        if ((font_data << Loop) & 0x80)
        {
          // scale x scale 크기로 픽셀 그리기
          for (uint8_t sy = 0; sy < scale; sy++)
          {
            for (uint8_t sx = 0; sx < scale; sx++)
            {
              lcdDrawPixel(x + (index_x * scale) + sx,
                         y + (i * scale) + sy,
                         textcolor);
            }
          }
        }
        index_x++;
      }
    }
  }
}
#endif

void disHanFontScale(int x, int y, han_font_t *FontPtr, uint16_t textcolor, uint8_t scale)
{
  uint16_t i, j, Loop;
  uint16_t FontSize = FontPtr->Size_Char;
  uint16_t index_x;

  if (FontSize > 2)
  {
    FontSize = 2;
  }

  // 16x16 폰트의 각 라인
  for (i = 0; i < 16; i++)
  {
    index_x = 0;
    // 각 바이트(가로 8픽셀)
    for (j = 0; j < FontSize; j++)
    {
      uint8_t font_data;
      font_data = FontPtr->FontBuffer[i * FontSize + j];

      // 각 비트(픽셀)
      for (Loop = 0; Loop < 8; Loop++)
      {
        if ((font_data << Loop) & 0x80)
        {
          // scale x scale 크기로 픽셀 그리기
          for (uint8_t sy = 0; sy < scale; sy++)
          {
            for (uint8_t sx = 0; sx < scale; sx++)
            {
              lcdDrawPixel(x + (index_x * scale) + sx,
                           y + (i * scale) + sy,
                           textcolor);
            }
          }
        }
        index_x++;
      }
    }
  }
}

// lcdPrintf 함수에서 사용할 수 있도록 수정된 버전의 한글 출력 함수
int disHanFontPrintf(int x, int y, han_font_t *FontPtr, uint16_t textcolor, uint8_t scale)
{
  disHanFontScale(x, y, FontPtr, textcolor, scale);

  // 다음 문자의 x 위치를 scale 값에 맞게 조정하여 반환
  return x + (16 * scale); // 16은 한글 폰트의 기본 폭
}

extern const uint16_t Font7x10[];

void disEngFont(int x, int y, char ch, lcd_font_t *font, uint16_t textcolor)
{
  uint32_t i, b, j;


  // We gaan door het font
  for (i = 0; i < font->height; i++)
  {
    b = font->data[(ch - 32) * font->height + i];
    for (j = 0; j < font->width; j++)
    {
      if ((b << j) & 0x8000)
      {
        lcdDrawPixel(x + j, (y + i), textcolor);
      }
    }
  }
}

void lcdSetFont(LcdFont font)
{
  lcd_font = font;
}

LcdFont lcdGetFont(void)
{
  return lcd_font;
}

LCD_OPT_DEF uint16_t lcdGetColorMix(uint16_t c1_, uint16_t c2_, uint8_t mix)
{
  uint16_t r, g, b;
  uint16_t ret;
  uint16_t c1;
  uint16_t c2;

#if 0
  c1 = ((c1_>>8) & 0x00FF) | ((c1_<<8) & 0xFF00);
  c2 = ((c2_>>8) & 0x00FF) | ((c2_<<8) & 0xFF00);
#else
  c1 = c1_;
  c2 = c2_;
#endif
  r = ((uint16_t)((uint16_t)GETR(c1) * mix + GETR(c2) * (255 - mix)) >> 8);
  g = ((uint16_t)((uint16_t)GETG(c1) * mix + GETG(c2) * (255 - mix)) >> 8);
  b = ((uint16_t)((uint16_t)GETB(c1) * mix + GETB(c2) * (255 - mix)) >> 8);

  ret = MAKECOL(r, g, b);


  // return ((ret>>8) & 0xFF) | ((ret<<8) & 0xFF00);;
  return ret;
}

LCD_OPT_DEF void lcdDrawPixelMix(int16_t x_pos, int16_t y_pos, uint32_t rgb_code, uint8_t mix)
{
  uint16_t color1, color2;

  if (x_pos < 0 || x_pos >= LCD_WIDTH) return;
  if (y_pos < 0 || y_pos >= LCD_HEIGHT) return;

  color1 = p_draw_frame_buf[y_pos * LCD_WIDTH + x_pos];
  color2 = rgb_code;

  p_draw_frame_buf[y_pos * LCD_WIDTH + x_pos] = lcdGetColorMix(color1, color2, 255 - mix);
}

#ifdef HW_LCD_LVGL
void lcdDrawImage(int16_t x, int16_t y, lcd_img_t *p_img)
{
  int16_t   w;
  int16_t   h;
  uint16_t *p_data;
  uint16_t  pixel;

  w      = p_img->header.w;
  h      = p_img->header.h;
  p_data = (uint16_t *)p_img->data;

  for (int yi = 0; yi < h; yi++)
  {
    for (int xi = 0; xi < w; xi++)
    {
      pixel = p_data[w * yi + xi];
      if (pixel != green)
      {
        lcdDrawPixel(x + xi, y + yi, pixel);
      }
    }
  }
}
#endif

int getCharacterCount(const char* str) {
    int count = 0;
    while (*str) {
        if ((*str & 0x80) == 0) {
            // ASCII 문자 (1바이트)
            count++;
            str++;
        } else if ((*str & 0xE0) == 0xE0) {
            // 한글 등 3바이트 문자
            count++;
            str += 3;
        } else {
            // 기타 멀티바이트 문자는 건너뛰기
            str++;
        }
    }
    return count;
}

// 문자가 영문자인지 확인하는 함수 추가
bool isEnglish(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

// 문자열의 실제 픽셀 너비를 계산하는 함수
int getStringWidth(const char* str) {
    int width = 0;
    while (*str) {
        if (*str == ' ') {
            // 공백은 16픽셀로 계산
            width += 16;
            str++;
        }
        else if ((*str & 0x80) == 0) {
            // ASCII 문자 (영문자는 16픽셀, 나머지는 32픽셀)
            width += (isEnglish(*str)) ? 16 : 32;
            str++;
        }
        else if ((*str & 0xE0) == 0xE0) {
            // 한글 등 3바이트 문자 (32픽셀)
            width += 32;
            str += 3;
        }
        else {
            str++;
        }
    }
    return width;
}

#ifdef _USE_HW_CLI
void cliLcd(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    cliPrintf("Driver : SSD1322\n");
    cliPrintf("Init   : %s\n", lcdIsInit() ? "OK" : "Fail");
    cliPrintf("Draw   : %s\n", lcd_request_draw ? "OK" : "Fail");
    cliPrintf("Width  : %d\n", lcdGetWidth());
    cliPrintf("Height : %d\n", lcdGetHeight());
    cliPrintf("BKL    : %d%%\n", lcdGetBackLight());
    ret = true;
  }
  if (args->argc == 3 && args->isStr(0, "pixel") == true)
  {
    uint16_t x     = args->getData(1);
    uint16_t y     = args->getData(2);
    uint16_t color = white;

    x = constrain(x, 0, LCD_WIDTH - 1);
    y = constrain(y, 0, LCD_HEIGHT - 1);

    lcdDrawPixel(x, y, color);
    lcdUpdateDraw();

    cliPrintf("Pixel : %d, %d, %d\n", x, y, color);

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "test") == true)
  {
    uint16_t o_x       = 0;
    uint8_t  o_y       = 0;
    uint8_t  test_mode = 0;
    uint32_t pre_time  = millis();

    while (cliKeepLoop())
    {
      if (lcdDrawAvailable() == true)
      {
        lcdClearBuffer(black);

        switch (test_mode)
        {
          case 0: // 기본 정보 및 이동하는 박스
            lcdSetFont(LCD_FONT_07x10);
            lcdPrintf(0, 0, white, "SSD1322 256x64 TEST");
            lcdPrintf(0, 16, white, "FPS : %d", lcdGetFps());

            // 이동하는 박스 (좌우)
            lcdDrawFillRect(o_x, 32, 32, 16, white);
            o_x += 4;
            if (o_x >= LCD_WIDTH - 32) o_x = 0;
            break;

          case 1: // 체커보드 패턴
            for (int y = 0; y < LCD_HEIGHT; y += 8)
            {
              for (int x = 0; x < LCD_WIDTH; x += 8)
              {
                if ((x + y) % 16 == 0)
                  lcdDrawFillRect(x, y, 8, 8, white);
              }
            }
            break;

          case 2: // 대각선 패턴
            for (int i = 0; i < LCD_WIDTH + LCD_HEIGHT; i += 8)
            {
              lcdDrawLine(0, i, i, 0, white);
            }
            break;

          case 3: // 텍스트 스크롤
            lcdSetFont(LCD_FONT_07x10);
            for (int i = 0; i < 4; i++)
            {
              int y_pos = (i * 16 + o_y) % LCD_HEIGHT;
              lcdPrintf(0, y_pos, white, "Scroll Text Line %d", i + 1);
            }
            o_y = (o_y + 1) % LCD_HEIGHT;
            break;
        }

        // 3초마다 테스트 모드 변경
        if (millis() - pre_time >= 3000)
        {
          pre_time  = millis();
          test_mode = (test_mode + 1) % 4;
          o_x       = 0;
          o_y       = 0;
        }

        lcdRequestDraw();
      }
    }

    lcdClearBuffer(black);
    lcdUpdateDraw();
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "test2") == true)
  {
    uint8_t  test_phase = 0;
    uint32_t pre_time   = millis();

    while (cliKeepLoop())
    {
      if (lcdDrawAvailable() == true)
      {
        lcdClearBuffer(black);

        switch (test_phase)
        {
          case 0: // 단일 픽셀 테스트
            // lcdPrintf(0, 0, white, "Pixel Test");
            // 코너 픽셀
            lcdDrawPixel(0, 0, white);
            lcdDrawPixel(255, 0, white);
            lcdDrawPixel(0, 63, white);
            lcdDrawPixel(255, 63, white);
            // 중앙 픽셀
            lcdDrawPixel(128, 32, white);
            break;

          case 1: // 수직선 테스트
            // lcdPrintf(0, 0, white, "Vertical Line Test");
            for (int x = 0; x < LCD_WIDTH; x += 32)
            {
              lcdDrawLine(x, 0, x, LCD_HEIGHT - 1, white);
            }
            break;

          case 2: // 수평선 테스트
            // lcdPrintf(0, 0, white, "Horizontal Line Test");
            for (int y = 0; y < LCD_HEIGHT; y += 8)
            {
              lcdDrawLine(0, y, LCD_WIDTH - 1, y, white);
            }
            break;

          case 3: // 대각선 테스트
            // lcdPrintf(0, 0, white, "Diagonal Line Test");
            lcdDrawLine(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, white);
            lcdDrawLine(LCD_WIDTH - 1, 0, 0, LCD_HEIGHT - 1, white);
            break;

          case 4: // 사각형 테스트
            // lcdPrintf(0, 0, white, "Rectangle Test");
            // 빈 사각형
            lcdDrawRect(10, 10, 50, 50, white);
            // 채워진 사각형
            lcdDrawFillRect(70, 10, 50, 50, white);
            // 테두리가 있는 채워진 사각형
            // lcdDrawFillRect(130, 10, 50, 50, white);
            // lcdDrawRect(130, 10, 50, 50, black);
            break;

          case 5: // 픽셀 패턴 테스트
            // lcdPrintf(0, 0, white, "Pattern Test");
            for (int y = 16; y < LCD_HEIGHT; y += 2)
            {
              for (int x = 0; x < LCD_WIDTH; x += 2)
              {
                lcdDrawPixel(x, y, white);
              }
            }
            break;
        }

        // 현재 테스트 페이즈 표시
        lcdPrintf(180, 0, white, "%d/5", test_phase);

        // 2초마다 다음 테스트로
        if (millis() - pre_time >= 2000)
        {
          pre_time   = millis();
          test_phase = (test_phase + 1) % 6;
        }

        lcdRequestDraw();
      }
    }

    lcdClearBuffer(black);
    lcdUpdateDraw();
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "test3") == true)
  {
    while (cliKeepLoop())
    {
      if (lcdDrawAvailable() == true)
      {
        lcdClearBuffer(black);

        lcdDrawPixel(128, 32, white);

        lcdRequestDraw();
      }
    }

    lcdClearBuffer(black);
    lcdUpdateDraw();

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "han") == true)
  {
    while (cliKeepLoop())
    {
      if (lcdDrawAvailable() == true)
      {
        lcdClearBuffer(black);

        lcdSetFont(LCD_FONT_HAN);

        lcdPrintf(0, 16, white, "안녕하세요");

        lcdRequestDraw();
        delay(100);
      }
    }

    // 테스트 종료 시 화면 클리어
    lcdClearBuffer(black);
    lcdUpdateDraw();

    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "bl") == true)
  {
    uint8_t bl_value;

    bl_value = args->getData(1);

    lcdSetBackLight(bl_value);

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "hanscale") == true)
  {
    while (cliKeepLoop())
    {
      if (lcdDrawAvailable() == true)
      {
        lcdClearBuffer(black);

        han_font_t FontBuf;
        char       test_str[] = "가";

        // 한글 폰트 로드
        hanFontLoad(test_str, &FontBuf);

        // 다양한 크기로 테스트
        disHanFontScale(0, 0, &FontBuf, white, 1);   // 원본 크기
        disHanFontScale(32, 0, &FontBuf, white, 2);  // 2배 크기
        disHanFontScale(80, 0, &FontBuf, white, 3);  // 3배 크기
        disHanFontScale(144, 0, &FontBuf, white, 4); // 4배 크기

        lcdRequestDraw();
        delay(100);
      }
    }

    lcdClearBuffer(black);
    lcdUpdateDraw();
    ret = true;
  }

  if (args->argc == 3 && args->isStr(0, "han_test") == true)
  {
    const char* messages[] = {   
      "바코드",
      "내전압",
      "공압센서",
      "LTM",
      "검사결과"
    };

    const char* status[] = {      
      "대기중",
      "검사중",
      "완료",
      "실패",
      "OK",
      "NG"
    };

    const uint32_t max_width =  LCD_WIDTH;

    // 인자 파싱
    uint32_t msg_idx = args->getData(1);     // 첫 번째 인자
    uint32_t status_idx = args->getData(2);  // 두 번째 인자

    uint32_t msg_cnt = sizeof(messages) / sizeof(messages[0]);
    uint32_t status_cnt = sizeof(status) / sizeof(status[0]);

    // 범위 체크
    msg_idx = msg_idx % msg_cnt;
    status_idx = status_idx % status_cnt;

    // 전체 문자열 조합 (message + " " + status)
    char combined_str[32];
    snprintf(combined_str, sizeof(combined_str), "%s %s", 
             messages[msg_idx], status[status_idx]);

    // 전체 문자열의 픽셀 너비 계산
    int total_width = getStringWidth(combined_str);
    
    // LCD 중앙에 배치하기 위한 시작 x 좌표 계산
    int start_x = (max_width - total_width) / 2;
    cliPrintf("start_x: %d, total_width: %d\n", start_x, total_width);

    if (lcdDrawAvailable() == true)
    {
      lcdClearBuffer(black);
      
      // 중앙 정렬된 위치에 전체 문자열 출력
      lcdPrintf(start_x, 16, white, "%s", combined_str);

      lcdRequestDraw();
    }
    
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "han_cycle") == true)
  {
    static const char* const messages[] = {"바코드","내전압","공압센서","LTM","검사결과"};
    static const char* const status[] = {"대기중","검사중","완료","실패","OK","NG"};
    
    // uint32_t time_ms = 1000;
    uint32_t idx = 0;
    uint32_t pre_time = millis();

        
    // time_ms = args->getData(1);
    // time_ms = constrain(time_ms, 100, 2000);

    while (cliKeepLoop())
    {
      if (lcdDrawAvailable() == true)
      {
        if (millis() - pre_time >= 500)
        {
          pre_time = millis();
          idx++;
          if (idx >= 30) idx = 0;  // 5 * 6 = 30 총 조합
        }

        char str[32];
        snprintf(str, sizeof(str), "%s %s", messages[idx/6], status[idx%6]);
        
        lcdClearBuffer(black);
        lcdPrintf((LCD_WIDTH - getStringWidth(str))/2, 16, white, str);
        lcdRequestDraw();
      }
      delay(100);
    }
    
    lcdClearBuffer(black);
    lcdUpdateDraw();
    ret = true;
  }

  if (args->isStr(0, "safety_test") == true)
  {
    //
    //  <<-- 안전센서 UI 관련 코드 시작점
    //
    uint32_t display_data = args->getData(1);     // 첫 번째 인자

    const char display_menu_name[] = "안전센서";

    char display_buffer[32] = {0x00,};
    char display_draw_number_flag[4] = {0x00,};

    logPrintf("[input debug] %b\n",display_data);

    for(int i = 0 ; i < 4 ; i++ ) //  4개의 상태값만 받을 예정
    {
      if(((display_data >> i) & 0x01) == 0x01 )
      {
        display_draw_number_flag[i] = 1;
      }
      else
      {
        display_draw_number_flag[i] = 0;
      }
    }

    //  각 라인은 top / bot 기준으로 y축 값을 의미.
    //  lef / rig는 x 축 값을 의미
    uint8_t top_line = 10;
    uint8_t bot_line = 50;
    uint8_t lef_line = 0;
    uint8_t rig_line = 0;

    //  16은 모든 string의 y 기준 값 >> define으로 변경 가능
    const uint8_t display_string_y_location = 16;

    snprintf(display_buffer, sizeof(display_buffer), "%s", display_menu_name);

    lcdClearBuffer(black);
    lcdPrintf( 1, display_string_y_location, white, display_buffer);

    //  1번 객체 시작점은 130, 전체 사각형 크기는 30x40
    lef_line = 130;
    rig_line = lef_line + 30;
    lcdDrawFillRect(lef_line ,top_line ,rig_line - lef_line ,bot_line - top_line ,white );
    if(display_draw_number_flag[0] == off)
    {
      lcdDrawFillRect(lef_line + 1, top_line + 1, rig_line - lef_line - 2, bot_line - top_line - 2,black );
      lcdPrintf( lef_line + 6, display_string_y_location, white, "1");
    }
    else if(display_draw_number_flag[0] == on)
    {
      lcdDrawFillRect(lef_line + 1, top_line + 1, rig_line - lef_line - 2, bot_line - top_line - 2,black );
      lcdDrawFillRect(lef_line + 2, top_line + 2, rig_line - lef_line - 4, bot_line - top_line - 4,white );
      lcdPrintf( lef_line + 6, display_string_y_location, black, "1");
    }

    //  2번 객체 시작점은 161, 전체 사각형 크기는 30x40
    lef_line = 161;
    rig_line = lef_line + 30;
    lcdDrawFillRect(lef_line ,top_line ,rig_line - lef_line ,bot_line - top_line ,white );
    if(display_draw_number_flag[1] == off)
    {
      lcdDrawFillRect(lef_line + 1, top_line + 1, rig_line - lef_line - 2, bot_line - top_line - 2,black );
      lcdPrintf( lef_line + 6, display_string_y_location, white, "2");
    }
    else if(display_draw_number_flag[1] == on)
    {
      lcdDrawFillRect(lef_line + 1, top_line + 1, rig_line - lef_line - 2, bot_line - top_line - 2,black );
      lcdDrawFillRect(lef_line + 2, top_line + 2, rig_line - lef_line - 4, bot_line - top_line - 4,white );
      lcdPrintf( lef_line + 6, display_string_y_location, black, "2");
    }

    //  3번 객체 시작점은 192, 전체 사각형 크기는 30x40
    lef_line = 192;
    rig_line = lef_line + 30;
    lcdDrawFillRect(lef_line ,top_line ,rig_line - lef_line ,bot_line - top_line ,white );
    if(display_draw_number_flag[2] == off)
    {
      lcdDrawFillRect(lef_line + 1, top_line + 1, rig_line - lef_line - 2, bot_line - top_line - 2,black );
      lcdPrintf( lef_line + 6, display_string_y_location, white, "3");
    }
    else if(display_draw_number_flag[2] == on)
    {
      lcdDrawFillRect(lef_line + 1, top_line + 1, rig_line - lef_line - 2, bot_line - top_line - 2,black );
      lcdDrawFillRect(lef_line + 2, top_line + 2, rig_line - lef_line - 4, bot_line - top_line - 4,white );
      lcdPrintf( lef_line + 6, display_string_y_location, black, "3");
    }

    //  4번 객체 시작점은 223, 전체 사각형 크기는 30x40
    lef_line = 223;
    rig_line = lef_line + 30;
    lcdDrawFillRect(lef_line ,top_line ,rig_line - lef_line ,bot_line - top_line ,white );
    if(display_draw_number_flag[3] == off)
    {
      lcdDrawFillRect(lef_line + 1, top_line + 1, rig_line - lef_line - 2, bot_line - top_line - 2,black );
      lcdPrintf( lef_line + 6, display_string_y_location, white, "4");
    }
    else if(display_draw_number_flag[3] == on)
    {
      lcdDrawFillRect(lef_line + 1, top_line + 1, rig_line - lef_line - 2, bot_line - top_line - 2,black );
      lcdDrawFillRect(lef_line + 2, top_line + 2, rig_line - lef_line - 4, bot_line - top_line - 4,white );
      lcdPrintf( lef_line + 6, display_string_y_location, black, "4");
    }

    //  최종 버퍼 그리기
    lcdRequestDraw();

    //
    //  <<-- 안전센서 UI 관련 코드 종단점
    //
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("lcd info\n");
    cliPrintf("lcd pixel [x] [y]\n");
    cliPrintf("lcd test\n");
    cliPrintf("lcd test2\n");
    cliPrintf("lcd test3\n");
    cliPrintf("lcd bl 0~100\n");
    cliPrintf("lcd han\n");
    cliPrintf("lcd hansize\n");
    cliPrintf("lcd hanscale\n");
    cliPrintf("lcd han_test [msg_idx] [status_idx]\n");
    cliPrintf("lcd han_cycle\n");
    cliPrintf("lcd safety_test <count>\n");
  }
}
#endif

#endif


