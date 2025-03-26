#ifndef HW_DEF_H_
#define HW_DEF_H_


#include "bsp.h"


#define _DEF_FIRMWATRE_VERSION "V241212R1"
#define _DEF_BOARD_NAME        "STM32G4-CGM-TEST-PRESSURE"

#define _USE_HW_PRESSURE
#define _USE_HW_FLASH

#define _USE_HW_US6300
#define HW_US6300_MAX_CH 4

#define _USE_HW_TCA9548
#define HW_TCA9548_MAX_CH 4

#define _USE_HW_LED
#define HW_LED_MAX_CH 3

#define _USE_HW_UART
#define HW_UART_MAX_CH   3
#define HW_UART_CH_DEBUG _DEF_UART1
#define HW_UART_CH_RS232 _DEF_UART2
#define HW_UART_CH_USB   _DEF_UART3

#define _USE_HW_LOG
#define HW_LOG_CH           HW_UART_CH_DEBUG
#define HW_LOG_BOOT_BUF_MAX 1024 // 2048
#define HW_LOG_LIST_BUF_MAX 1024 // 4096

#define _USE_HW_CLI
#define HW_CLI_CMD_LIST_MAX 32
#define HW_CLI_CMD_NAME_MAX 16
#define HW_CLI_LINE_HIS_MAX 8
#define HW_CLI_LINE_BUF_MAX 64

#define _USE_HW_CLI_GUI
#define HW_CLI_GUI_WIDTH  80
#define HW_CLI_GUI_HEIGHT 24

#define _USE_HW_SWTIMER
#define HW_SWTIMER_MAX_CH 8

#define _USE_HW_USB
#define _USE_HW_CDC
#define HW_USE_CDC 1
#define HW_USE_MSC 0

#define _USE_HW_CAN
#define HW_CAN_MAX_CH         1
#define HW_CAN_MSG_RX_BUF_MAX 32

#define _USE_HW_ADC
#define HW_ADC_MAX_CH ADC_PIN_MAX

#define _USE_HW_GPIO
#define HW_GPIO_MAX_CH 9

#define _USE_HW_I2C
#define HW_I2C_MAX_CH  1
#define HW_I2C_CH_OLED _DEF_I2C1


#define FLASH_CFG_TAG_ADDR 0x0801F000
#define FLASH_CFG_TAG_SIZE 2048

#define FLASH_CFG_ADDR 0x0801F800
#define FLASH_CFG_SIZE 2048


typedef enum
{
  mGPIO_OUT1 = 1,
  mGPIO_IN1,
  mGPIO_IN2,
  mGPIO_IN3,
  mGPIO_IN4,
} GpioPinName_t;


typedef enum
{
  mADC1 = 0,
  mADC2,
  mADC3,
  mADC4,

  ADC_PIN_MAX
} AdcPinName_t;

#endif
