#ifndef ST7789_H_
#define ST7789_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hw_def.h"


#ifdef _USE_HW_SSD1322

#include "lcd.h"
#include "ssd1322_regs.h"


typedef struct
{
  uint32_t req_time;
} ssd1322_info_t;


bool ssd1322Init(void);
bool ssd1322InitDriver(lcd_driver_t *p_driver);
uint8_t *ssd1322GetBuffer(void);
bool ssd1322DrawGrayScale(const uint8_t *gray_data);

#endif

#ifdef __cplusplus
}
#endif

#endif
