/*
 * ssd1322.c
 *
 *  Created on: 2020. 12. 30.
 *      Author: baram
 */


#include "lcd/ssd1322.h"
#include "gpio.h"
#include "lcd/ssd1322_regs.h"
#include "spi.h"


#ifdef _USE_HW_SSD1322
#include "lcd.h"
#endif

#define SSD1322_WIDTH  HW_LCD_WIDTH
#define SSD1322_HEIGHT HW_LCD_HEIGHT

static uint8_t spi_ch   = _DEF_SPI1;
static uint8_t gpio_dc  = GPIO_CH_LCD_DC;
static uint8_t gpio_cs  = GPIO_CH_LCD_CS;
static uint8_t gpio_rst = GPIO_CH_LCD_RST;

static void (*frameCallBack)(void) = NULL;


static bool     ssd1322Reset(void);
static void     ssd1322SetWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
static uint16_t ssd1322GetWidth(void);
static uint16_t ssd1322GetHeight(void);
static bool     ssd1322SendBuffer(uint8_t *p_data, uint32_t length, uint32_t timeout_ms);
static bool     ssd1322SetCallBack(void (*p_func)(void));
static void     ssd1322Fill(uint16_t color);
static bool     ssd1322UpdateDraw(void);

static uint8_t ssd1322_buffer[HW_LCD_WIDTH * HW_LCD_HEIGHT / 8];

bool ssd1322Init(void)
{
  bool ret = true;

  gpioPinWrite(gpio_dc, _DEF_HIGH);
  gpioPinWrite(gpio_cs, _DEF_HIGH);
  gpioPinWrite(gpio_rst, _DEF_LOW);
  delay(10);
  gpioPinWrite(gpio_rst, _DEF_HIGH);
  delay(10);

  ret &= ssd1322Reset();

  logPrintf("[%s] ssd1322Init()\n", ret ? "OK" : "NG");

  return ret;
}

bool ssd1322InitDriver(lcd_driver_t *p_driver)
{
  p_driver->init        = ssd1322Init;
  p_driver->reset       = ssd1322Reset;
  p_driver->setWindow   = ssd1322SetWindow;
  p_driver->getWidth    = ssd1322GetWidth;
  p_driver->getHeight   = ssd1322GetHeight;
  p_driver->setCallBack = ssd1322SetCallBack;
  p_driver->sendBuffer  = ssd1322SendBuffer; // ssd1322UpdateDraw
  return true;
}

uint8_t *ssd1322GetBuffer(void)
{
  return ssd1322_buffer;
}

bool ssd1322WriteCmd(uint8_t cmd)
{
  bool ret;

  gpioPinWrite(gpio_cs, _DEF_LOW);
  gpioPinWrite(gpio_dc, _DEF_LOW);
  ret = spiTransfer(spi_ch, (uint8_t *)&cmd, NULL, 1, 50);
  gpioPinWrite(gpio_cs, _DEF_HIGH);

  if (ret == false)
  {
    logPrintf("[NG] ssd1322WriteCmd() %02X\n", cmd);
  }

  return ret;
}

bool ssd1322WriteData(uint8_t data)
{
  bool ret;

  gpioPinWrite(gpio_cs, _DEF_LOW);
  gpioPinWrite(gpio_dc, _DEF_HIGH);
  ret = spiTransfer(spi_ch, (uint8_t *)&data, NULL, 1, 50);
  gpioPinWrite(gpio_cs, _DEF_HIGH);


  if (ret == false)
  {
    logPrintf("[NG] ssd1322WriteData() %02X\n", data);
  }

  return ret;
}

// bool ssd1322Reset(void)
// {
//   // bool ret;
//   // SPI 초기화
//   // ret = spiBegin(spi_ch);
//   // if (ret != true)
//   // {
//   //   return false;
//   // }

//   // SSD1322 초기화 명령어 전송
//   ssd1322WriteCmd(0xFD); // 명령 잠금 설정
//   ssd1322WriteCmd(0x12); // OLED 드라이버 IC 잠금 해제
//   ssd1322WriteCmd(0xAE); // 디스플레이 끄기
//   ssd1322WriteCmd(0xB3); // 디스플레이 클럭 분주비 및 오실레이터 주파수 설정
//   ssd1322WriteCmd(0x91); // 기본값 사용
//   ssd1322WriteCmd(0xCA); // 다중화 비율 설정 (MUX Ratio)
//   ssd1322WriteCmd(0x3F); // 64 라인 (63)
//   ssd1322WriteCmd(0xA2); // 디스플레이 오프셋 설정
//   ssd1322WriteCmd(0x00); // 오프셋 없음
//   ssd1322WriteCmd(0xA1); // 디스플레이 시작 라인 설정
//   ssd1322WriteCmd(0x00); // 첫 번째 라인에서 시작
//   ssd1322WriteCmd(0xA0); // 리맵 및 듀얼 COM 라인 모드 설정
//   ssd1322WriteCmd(0x14); // 니블 리맵 활성화, 수평 증가 모드
//   ssd1322WriteCmd(0x11); // 듀얼 COM 모드 활성화
//   ssd1322WriteCmd(0xAB); // 기능 선택 (내부 VDD 사용)
//   ssd1322WriteCmd(0x01);
//   ssd1322WriteCmd(0xB4); // 디스플레이 향상 A 설정
//   ssd1322WriteCmd(0xA0);
//   ssd1322WriteCmd(0xFD);
//   ssd1322WriteCmd(0xC1); // 대비 전류 설정
//   ssd1322WriteCmd(0x9F);
//   ssd1322WriteCmd(0xC7); // 마스터 대비 전류 제어
//   ssd1322WriteCmd(0x0F);
//   ssd1322WriteCmd(0xB1); // 페이즈 길이 설정
//   ssd1322WriteCmd(0xE2);
//   ssd1322WriteCmd(0xD1); // 디스플레이 향상 B 설정
//   ssd1322WriteCmd(0x82);
//   ssd1322WriteCmd(0x20);
//   ssd1322WriteCmd(0xBB); // 프리차지 전압 설정
//   ssd1322WriteCmd(0x1F);
//   ssd1322WriteCmd(0xB6); // 두 번째 프리차지 기간 설정
//   ssd1322WriteCmd(0x08);
//   ssd1322WriteCmd(0xBE); // VCOMH 전압 설정
//   ssd1322WriteCmd(0x07);
//   ssd1322WriteCmd(0xA6); // 일반 디스플레이 모드

//   ssd1322WriteCmd(0xAF); // 디스플레이 켜기

//   ssd1322Fill(white);
//   ssd1322UpdateDraw();

//   return true;
// }

bool ssd1322Reset(void)
{
  bool ret;
  // SPI 초기화
  ret = spiBegin(spi_ch);
  if (ret != true)
  {
    return false;
  }

  // ssd1322WriteCmd(SSD1322_SETCOMMANDLOCK);           // 명령 잠금 설정
  // ssd1322WriteData(0x12);                            // OLED 드라이버 IC 잠금 해제
  // ssd1322WriteCmd(SSD1322_DISPLAYOFF);               // 디스플레이 끄기
  // ssd1322WriteCmd(SSD1322_SETCLOCKDIVIDER);          // 디스플레이 클럭 분주비 및 오실레이터 주파수 설정
  // ssd1322WriteData(0x91);                            // 기본값 사용
  // ssd1322WriteCmd(SSD1322_SETMUXRATIO);              // 다중화 비율 설정 (MUX Ratio)
  // ssd1322WriteData(0x3F);                            // 64 라인 (63)
  // ssd1322WriteCmd(SSD1322_SETDISPLAYOFFSET);         // 디스플레이 오프셋 설정
  // ssd1322WriteData(0x00);                            // 오프셋 없음
  // ssd1322WriteCmd(SSD1322_SETSTARTLINE);             // 디스플레이 시작 라인 설정
  // ssd1322WriteData(0x00);                            // 첫 번째 라인에서 시작
  // ssd1322WriteCmd(SSD1322_SETREMAP);                 // 리맵 및 듀얼 COM 라인 모드 설정
  // ssd1322WriteData(0x14);                            // 니블 리맵 활성화, 수평 증가 모드
  // ssd1322WriteData(0x11);                            // 듀얼 COM 모드 활성화
  // ssd1322WriteCmd(SSD1322_SETGPIO);                  // GPIO 핀 입력 비활성화
  // ssd1322WriteData(0x00);
  // ssd1322WriteCmd(SSD1322_FUNCTIONSEL);              // 기능 선택 (외부 VDD 사용)
  // ssd1322WriteData(0x01);
  // ssd1322WriteCmd(SSD1322_DISPLAYENHANCE);           // 디스플레이 향상 A 설정
  // ssd1322WriteData(0xA0);
  // ssd1322WriteData(0xFD);
  // ssd1322WriteCmd(SSD1322_SETCONTRASTCURRENT);       // 대비 전류 설정
  // ssd1322WriteData(0xFF);
  // ssd1322WriteCmd(SSD1322_MASTERCURRENTCONTROL);     // 마스터 대비 전류 제어
  // ssd1322WriteData(0x0F);
  // ssd1322WriteCmd(SSD1322_SELECTDEFAULTGRAYSCALE);   // 기본 그레이스케일 설정
  // ssd1322WriteCmd(SSD1322_SETPHASELENGTH);           // 페이즈 길이 설정
  // ssd1322WriteData(0xE2);
  // ssd1322WriteCmd(SSD1322_DISPLAYENHANCEB);          // 디스플레이 향상 B 설정
  // ssd1322WriteData(0x82);
  // ssd1322WriteData(0x20);
  // ssd1322WriteCmd(SSD1322_SETPRECHARGEVOLTAGE);      // 프리차지 전압 설정
  // ssd1322WriteData(0x1F);
  // ssd1322WriteCmd(SSD1322_SETSECONDPRECHARGEPERIOD); // 두 번째 프리차지 기간 설정
  // ssd1322WriteData(0x08);
  // ssd1322WriteCmd(SSD1322_SETVCOMH);                 // VCOMH 전압 설정
  // ssd1322WriteData(0x07);
  // ssd1322WriteCmd(SSD1322_NORMALDISPLAY);            // 일반 디스플레이 모드
  // ssd1322WriteCmd(SSD1322_EXITPARTIALDISPLAY);       // 부분 디스플레이 모드 종료
  // delay(10);
  // ssd1322WriteCmd(SSD1322_ENTIREDISPLAYON);
  // delay(50);

  // ssd1322WriteCmd(SSD1322_ENTIREDISPLAYON);
  // ssd1322Fill(white);
  // ssd1322UpdateDraw();

  ssd1322WriteCmd(SSD1322_SETCOMMANDLOCK);
  ssd1322WriteData(0x12);
  ssd1322WriteCmd(SSD1322_DISPLAYOFF);
  ssd1322WriteCmd(SSD1322_SETCLOCKDIVIDER);
  ssd1322WriteData(0x91);
  ssd1322WriteCmd(SSD1322_SETMUXRATIO);
  ssd1322WriteData(0x3F);
  ssd1322WriteCmd(SSD1322_SETDISPLAYOFFSET);
  ssd1322WriteData(0x00);
  ssd1322WriteCmd(SSD1322_SETSTARTLINE);
  ssd1322WriteData(0x00);
  ssd1322WriteCmd(SSD1322_SETREMAP);
  ssd1322WriteData(0x14);
  ssd1322WriteData(0x11);
  ssd1322WriteCmd(SSD1322_SETGPIO);
  ssd1322WriteData(0x00);
  ssd1322WriteCmd(SSD1322_FUNCTIONSEL);
  ssd1322WriteData(0x01);
  ssd1322WriteCmd(SSD1322_DISPLAYENHANCE);
  ssd1322WriteData(0xA0);
  ssd1322WriteData(0xFD);
  ssd1322WriteCmd(SSD1322_SETCONTRASTCURRENT);
  ssd1322WriteData(0xFF);
  ssd1322WriteCmd(SSD1322_MASTERCURRENTCONTROL);
  ssd1322WriteData(0x0F);
  ssd1322WriteCmd(SSD1322_SELECTDEFAULTGRAYSCALE);
  ssd1322WriteCmd(SSD1322_SETPHASELENGTH);
  ssd1322WriteData(0xE2);
  ssd1322WriteCmd(SSD1322_DISPLAYENHANCEB);
  ssd1322WriteData(0x82);
  ssd1322WriteData(0x20);
  ssd1322WriteCmd(SSD1322_SETPRECHARGEVOLTAGE);
  ssd1322WriteData(0x1F);
  ssd1322WriteCmd(SSD1322_SETSECONDPRECHARGEPERIOD);
  ssd1322WriteData(0x08);
  ssd1322WriteCmd(SSD1322_SETVCOMH);
  ssd1322WriteData(0x07);
  ssd1322WriteCmd(SSD1322_NORMALDISPLAY);
  ssd1322WriteCmd(SSD1322_EXITPARTIALDISPLAY);
  delay(10);
  ssd1322WriteCmd(SSD1322_DISPLAYON);
  delay(50);

  // 모든 픽셀 ON (테스트용)
  // ssd1322WriteCmd(SSD1322_ENTIREDISPLAYON);  // 모든 픽셀 강제 ON
  // delay(10);

  ssd1322Fill(black);
  ssd1322UpdateDraw();

  return true;
}

void ssd1322SetWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
  // ssd1322WriteCmd(SSD1322_SET_COLUMN_ADDRESS); // set columns range
  // ssd1322WriteData(28 + x0);
  // ssd1322WriteData(28 + x1);
  // ssd1322WriteCmd(SSD1322_SET_ROW_ADDRESS);    // set rows range
  // ssd1322WriteData(y0);
  // ssd1322WriteData(y1);
}

uint16_t ssd1322GetWidth(void)
{
  return SSD1322_WIDTH;
}

uint16_t ssd1322GetHeight(void)
{
  return SSD1322_HEIGHT;
}

bool ssd1322SendBuffer(uint8_t *p_data, uint32_t length, uint32_t timeout_ms)
{
  bool ret = false;

  ret = ssd1322UpdateDraw();

  if (frameCallBack != NULL)
  {
    frameCallBack();
  }

  return ret;
}

bool ssd1322SetCallBack(void (*p_func)(void))
{
  frameCallBack = p_func;

  return true;
}

void ssd1322Fill(uint16_t color)
{
  if (color > 0)
  {
    memset(ssd1322_buffer, 0xFF, LCD_WIDTH * LCD_HEIGHT / 8);
  }
  else
  {
    memset(ssd1322_buffer, 0x00, LCD_WIDTH * LCD_HEIGHT / 8);
  }
}

bool ssd1322UpdateDraw(void)
{
  bool ret = true;

  // 1. 디스플레이 영역 설정 - set window
  
  ssd1322WriteCmd(SSD1322_SETROWADDR);
  ssd1322WriteData(0);
  ssd1322WriteData(63);

  ssd1322WriteCmd(SSD1322_SETCOLUMNADDR);
  ssd1322WriteData(28);
  ssd1322WriteData(91);

  // 2. 1비트 모노크롬 데이터를 4비트 그레이스케일로 변환
  uint8_t converted_buffer[256 * 64 / 2];                
  memset(converted_buffer, 0, sizeof(converted_buffer)); 

  for (uint32_t i = 0; i < (256 * 64 / 8); i++)
  {
    uint8_t byte = ssd1322_buffer[i];                    

    // MSB부터 처리하도록 수정
    for (int j = 7; j >= 0; j--)
    {
      uint8_t bit   = (byte >> j) & 0x01;
      uint8_t value = bit ? 0x0F : 0x00;  // 1->0x0F (흰색), 0->0x00 (검정)

      uint32_t out_pos = (i * 8 + (7-j)) / 2;  // 비트 위치 조정
      if ((i * 8 + (7-j)) % 2 == 0)
      {
        converted_buffer[out_pos] = value << 4;
      }
      else
      {
        converted_buffer[out_pos] |= value;
      }
    }
  }

  // 3. 변환된 데이터 전송
  ssd1322WriteCmd(SSD1322_WRITERAM);
  gpioPinWrite(gpio_cs, _DEF_LOW);
  gpioPinWrite(gpio_dc, _DEF_HIGH);
  ret = spiTransfer(spi_ch, converted_buffer, NULL, sizeof(converted_buffer), 100);
  gpioPinWrite(gpio_cs, _DEF_HIGH);

  return ret;
}

// 4비트 그레이스케일 이미지 데이터 직접 출력
bool ssd1322DrawGrayScale(const uint8_t *gray_data)
{
  bool ret = true;

  // 1. 디스플레이 영역 설정
  ssd1322WriteCmd(SSD1322_SETROWADDR);
  ssd1322WriteData(0);  // 시작 행
  ssd1322WriteData(63); // 종료 행 (64-1)

  ssd1322WriteCmd(SSD1322_SETCOLUMNADDR);
  ssd1322WriteData(28); // 시작 열 (하드웨어 오프셋)
  ssd1322WriteData(91); // 종료 열 (28 + 256/4 - 1)

  // 2. RAM 쓰기 시작
  ssd1322WriteCmd(SSD1322_WRITERAM);
  gpioPinWrite(gpio_cs, _DEF_LOW);
  gpioPinWrite(gpio_dc, _DEF_HIGH);

  // 3. 4비트 그레이스케일 데이터 직접 전송 (8KB)
  ret = spiTransfer(spi_ch, (uint8_t *)gray_data, NULL, 256 * 64 / 2, 100);

  gpioPinWrite(gpio_cs, _DEF_HIGH);

  return ret;
}
