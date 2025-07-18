#ifndef HW_DEF_H_
#define HW_DEF_H_


#include "bsp.h"


#define _DEF_FIRMWATRE_VERSION "V241212R1"
#define _DEF_BOARD_NAME        "STM32G4-CGM-TEST-SIGNTOWER"


#define _USE_HW_SIGNTOWER

#define _USE_HW_LED
#define HW_LED_MAX_CH 3

#define _USE_HW_UART
#define HW_UART_MAX_CH   4
#define HW_UART_CH_DEBUG _DEF_UART1
#define HW_UART_CH_RS232 _DEF_UART2
#define HW_UART_CH_RS485 _DEF_UART3
#define HW_UART_CH_USB   _DEF_UART4

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

typedef enum
{
  P_OUT1, // RED
  P_OUT2, // YELLOW
  P_OUT3, // GREEN
  P_OUT4, // X
  P_OUT5, // BUZZER
  P_OUT6, // X
  P_OUT7, // X
  P_OUT8, // BLINK_ENABLE
  ID0,
  ID1,
  ID2,

  GPIO_PIN_MAX
} GpioPinName_t;

#endif
