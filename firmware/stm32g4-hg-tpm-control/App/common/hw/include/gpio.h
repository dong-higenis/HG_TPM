#ifndef GPIO_H_
#define GPIO_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "hw_def.h"


#ifdef _USE_HW_GPIO


#define GPIO_MAX_CH     HW_GPIO_MAX_CH

typedef enum
{
  GPIO_CH_STATUS_LED,
  GPIO_CH_LINK_LED,
  //GPIO_CH_ERR_LED,
  GPIO_CH_P_LED1, // green
  GPIO_CH_P_LED2, // blue
  GPIO_CH_P_LED3, // red
  GPIO_CH_P_LED4, // estop
  GPIO_CH_P_IN1,
  GPIO_CH_P_IN2,
  GPIO_CH_P_IN3,
  GPIO_CH_P_IN4,
  GPIO_CH_LCD_CS,
  GPIO_CH_LCD_DC,  
  GPIO_CH_LCD_RST,
  GPIO_CH_CAN_STB,
  GPIO_CH_SD_CD,
  GPIO_CH_MAX,
} gpio_ch_t;

bool gpioInit(void);
bool gpioPinMode(uint8_t ch, uint8_t mode);
void gpioPinWrite(uint8_t ch, bool value);
bool gpioPinRead(uint8_t ch);
void gpioPinToggle(uint8_t ch);


#endif

#ifdef __cplusplus
}
#endif

#endif 
