#include "ap_oled.h"

static uint8_t frame_buffer[OLED_WIDTH * OLED_HEIGHT / 2]; // 4bit grayscale
static bool oled_inited = false;

bool apOledInit(void)
{
  bool ret = true;

  ret = hwOledInit();

  if (ret)
  {
    apOledClear(0x00);

    oled_inited = true;
  }

  return ret;
}

void apOledClear(uint8_t gray_level)
{
  memset(frame_buffer, gray_level, sizeof(frame_buffer));
}

void apOledFill(uint8_t gray_level)
{
  apOledClear(gray_level);
  apOledUpdate();
}

void apOledUpdate(void)
{
  if (!oled_inited)
  {
    return;
  }

  hwOledSetWindow(OLED_COL_START, OLED_COL_END, OLED_ROW_START, OLED_ROW_END);
  hwOledSendData(frame_buffer, sizeof(frame_buffer));
}

void apOledDrawPixel(int x, int y, uint8_t gray)
{
  if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT)
  {
    return;
  }

  uint32_t index = (y * OLED_WIDTH + x) / 2;

  if (x % 2 == 0)
  {
    frame_buffer[index] = (frame_buffer[index] & 0x0F) | (gray << 4);
  }
  else
  {
    frame_buffer[index] = (frame_buffer[index] & 0xF0) | (gray & 0x0F);
  }
}

void apOledDrawChar(int x, int y, char ch, const FontDef *font, uint8_t gray)
{
  if (!font || ch < 32 || ch > 126)
  {
    return;
  }
  if (gray > 15)
  {
    gray = 15;
  }

  if (x < 0 || (x + font->width) > OLED_WIDTH)
  {
    return;
  }

  if (y < 0 || (y + font->height) > OLED_HEIGHT)
  {
    return;
  }
  uint32_t char_index = ch - 32;

  if (font->width <= 16)
  {
    const uint16_t *char_data_16 = (const uint16_t *)&font->data[char_index * font->height];

    for (int row = 0; row < font->height; row++)
    {
      uint16_t mask = char_data_16[row];

      for (int col = 0; col < font->width; col++)
      {
        if (mask & (1 << (15 - col)))
        {
          apOledDrawPixel(x + col, y + row, gray);
        }
      }
    }
  }
  else
  {
    const uint8_t *char_data = &font->data[char_index * font->height * ((font->width + 7) / 8)];

    for (int row = 0; row < font->height; row++)
    {
      for (int col = 0; col < font->width; col++)
      {
        uint8_t byte_index = row * ((font->width + 7) / 8) + (col / 8);
        uint8_t bit_index = 7 - (col % 8);

        if (char_data[byte_index] & (1 << bit_index))
        {
          apOledDrawPixel(x + col, y + row, gray);
        }
      }
    }
  }
}

void apOledDrawString(int x, int y, const char *str, const FontDef *font, uint8_t gray)
{

  if (!str || !font)
  {
    return;
  }

  int current_x = x;

  while (*str)
  {
    if (*str == '\n')
    {
      current_x = x;
      y += font->height + 2;
    }
    else if (*str == '\r')
    {
    }
    else
    {
      apOledDrawChar(current_x, y, *str, font, gray);
      current_x += font->width;

      // 화면 끝에 도달하면 자동 줄바꿈
      if (current_x + font->width > OLED_WIDTH)
      {
        current_x = x;
        y += font->height + 2;
      }
    }
    str++;
  }
}

void apOledDrawRect(int x, int y, int width, int height, uint8_t gray)
{
  // 상단 가로선
  for (int i = 0; i < width; i++)
  {
    apOledDrawPixel(x + i, y, gray);
  }

  // 하단 가로선
  for (int i = 0; i < width; i++)
  {
    apOledDrawPixel(x + i, y + height - 1, gray);
  }

  // 좌측 세로선
  for (int i = 0; i < height; i++)
  {
    apOledDrawPixel(x, y + i, gray);
  }

  // 우측 세로선
  for (int i = 0; i < height; i++)
  {
    apOledDrawPixel(x + width - 1, y + i, gray);
  }
}

void apOledDrawFilledRect(int x, int y, int width, int height, uint8_t gray)
{
  for (int row = 0; row < height; row++)
  {
    for (int col = 0; col < width; col++)
    {
      apOledDrawPixel(x + col, y + row, gray);
    }
  }
}

void apOledDrawLine(int x1, int y1, int x2, int y2, uint8_t gray)
{
  int dx = abs(x2 - x1);
  int dy = abs(y2 - y1);
  int sx = (x1 < x2) ? 1 : -1;
  int sy = (y1 < y2) ? 1 : -1;
  int err = dx - dy;

  while (1)
  {
    apOledDrawPixel(x1, y1, gray);

    if (x1 == x2 && y1 == y2)
    {
      break;
    }
    int e2 = 2 * err;
    if (e2 > -dy)
    {
      err -= dy;
      x1 += sx;
    }
    if (e2 < dx)
    {
      err += dx;
      y1 += sy;
    }
  }
}

void apOledDrawBitmap(uint8_t *bmp_data, uint16_t width, uint16_t height, uint8_t x, uint8_t y)
{
  if (!bmp_data)
  {
    return;
  }

  for (int row = 0; row < height; row++)
  {
    for (int col = 0; col < width; col++)
    {
      if ((x + col) < OLED_WIDTH && (y + row) < OLED_HEIGHT)
      {
        // 비트맵 데이터가 4비트 그레이스케일이라고 가정
        uint32_t pixel_index = row * width + col;
        uint8_t gray_value;

        if (pixel_index % 2 == 0)
        {
          gray_value = (bmp_data[pixel_index / 2] >> 4) & 0x0F;
        }
        else
        {
          gray_value = bmp_data[pixel_index / 2] & 0x0F;
        }

        apOledDrawPixel(x + col, y + row, gray_value);
      }
    }
  }
}

void apOledDrawBitmapCenter(uint8_t *bmp_data, uint16_t width, uint16_t height)
{
  if (!bmp_data)
  {
    return;
  }

  int center_x = (OLED_WIDTH - width) / 2;
  int center_y = (OLED_HEIGHT - height) / 2;

  apOledDrawBitmap(bmp_data, width, height, center_x, center_y);
}

void apOledDisplayImageFromSD(char *file_name)
{
  if (!file_name)
  {
    return;
  }
  FIL fil;
  FRESULT fres = f_open(&fil, file_name, FA_READ);

  if (fres != FR_OK)
  {
    return;
  }

  // BMP 헤더 읽기 (54바이트)
  uint8_t bmp_header[54];
  UINT bytes_read;

  fres = f_read(&fil, bmp_header, 54, &bytes_read);

  if (fres != FR_OK || bytes_read != 54)
  {
    f_close(&fil);
    return;
  }

  // BMP 파일 확인
  if (bmp_header[0] != 'B' || bmp_header[1] != 'M')
  {
    f_close(&fil);
    return;
  }

  // 이미지 크기 정보 읽기
  uint32_t width = *(uint32_t *)&bmp_header[18];
  uint32_t height = *(uint32_t *)&bmp_header[22];
  uint16_t bpp = *(uint16_t *)&bmp_header[28];

  // 24비트 또는 32비트 BMP만 지원
  if (bpp != 24 && bpp != 32)
  {
    f_close(&fil);
    return;
  }

  // 픽셀 데이터 오프셋으로 이동
  uint32_t data_offset = *(uint32_t *)&bmp_header[10];
  fres = f_lseek(&fil, data_offset);

  if (fres != FR_OK)
  {
    f_close(&fil);
    return;
  }

  // 화면 클리어
  apOledClear(0x00);

  // BMP는 아래쪽부터 저장되므로 y좌표를 뒤집어서 읽기
  uint32_t bytes_per_pixel = bpp / 8;
  uint32_t row_size = ((width * bytes_per_pixel + 3) / 4) * 4; // 4바이트 정렬

  for (int y = height - 1; y >= 0; y--)
  {
    uint8_t row_buffer[row_size];
    fres = f_read(&fil, row_buffer, row_size, &bytes_read);

    if (fres != FR_OK)
    {
      break;
    }

    for (uint32_t x = 0; x < width && x < OLED_WIDTH; x++)
    {
      if (y < OLED_HEIGHT)
      {
        uint32_t pixel_offset = x * bytes_per_pixel;
        uint8_t b = row_buffer[pixel_offset];
        uint8_t g = row_buffer[pixel_offset + 1];
        uint8_t r = row_buffer[pixel_offset + 2];

        // RGB를 그레이스케일로 변환 (ITU-R BT.709)
        uint8_t gray = (uint8_t)((0.2126f * r + 0.7152f * g + 0.0722f * b) / 16);
        if (gray > 15)
        {
          gray = 15;
        }
        apOledDrawPixel(x, y, gray);
      }
    }
  }

  f_close(&fil);
  apOledUpdate();
}

void apOledSetCameraMode(void)
{
  // 카메라 모드 설정 (특별한 디스플레이 모드)
  hwOledSendCmd(0xA0);       // Set Re-map
  uint8_t remap_data = 0x14; // Horizontal address increment, disable column address re-map
  hwOledSendData(&remap_data, 1);

  hwOledSendCmd(0xA1); // Set Display Start Line
  uint8_t start_line = 0x00;
  hwOledSendData(&start_line, 1);
}

void apOledTest(void)
{
  // 테스트 패턴 1: 그라데이션
  apOledClear(0x00);

  for (int x = 0; x < OLED_WIDTH; x++)
  {
    uint8_t gray_level = (x * 15) / OLED_WIDTH;
    apOledDrawLine(x, 0, x, 20, gray_level);
  }

  // 테스트 패턴 2: 체크보드
  for (int y = 25; y < 45; y++)
  {
    for (int x = 0; x < OLED_WIDTH; x += 8)
    {
      uint8_t gray = ((x / 8) + (y / 8)) % 2 ? 0x0F : 0x00;
      apOledDrawFilledRect(x, y, 8, 8, gray);
    }
  }
  // 테스트 패턴 3: 텍스트
  apOledDrawString(10, 50, "Baram Style OLED Demo", &font_07x10, 15);

  // 테스트 패턴 4: 테두리
  apOledDrawRect(0, 0, OLED_WIDTH - 1, OLED_HEIGHT - 1, 0x0F);

  apOledUpdate();

  HAL_Delay(2000);

  // 애니메이션 테스트
  for (int i = 0; i < 50; i++)
  {
    apOledClear(0x00);
    apOledDrawFilledRect(i * 4, 20, 20, 20, 0x0F);
    apOledDrawString(10, 45, "Moving Box", &font_07x10, 15);
    apOledUpdate();
    HAL_Delay(100);
  }
}
