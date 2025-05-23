#ifndef HW_DEF_H_
#define HW_DEF_H_


#include "bsp.h"


#define _DEF_FIRMWATRE_VERSION "V241127R1"
#define _DEF_BOARD_NAME        "STM32G4-CGM-TEST-MAIN-FDCAN"


#define _USE_HW_LED
#define HW_LED_MAX_CH   3
#define HW_LED_CH_DEBUG _DEF_LED2
#define HW_LED_CH_CAN   _DEF_LED3

#define _USE_HW_UART
#define HW_UART_MAX_CH   4
#define HW_UART_CH_DEBUG _DEF_UART1
#define HW_UART_CH_RS232 _DEF_UART2
#define HW_UART_CH_QKEY  _DEF_UART3
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

#define _USE_HW_BUTTON
#define HW_BUTTON_MAX_CH 6
#define HW_BUTTON_CH_S1  _DEF_BUTTON1
#define HW_BUTTON_CH_S2  _DEF_BUTTON2
#define HW_BUTTON_CH_S3  _DEF_BUTTON3
#define HW_BUTTON_CH_S4  _DEF_BUTTON4
#define HW_BUTTON_CH_S5  _DEF_BUTTON5
#define HW_BUTTON_CH_S6  _DEF_BUTTON6

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
#define HW_GPIO_MAX_CH 17

#define _USE_HW_CMD
#define HW_CMD_MAX_DATA_LENGTH 1024

typedef enum
{
  mADC1 = 0,
  mADC2,
  mADC3,

  ADC_PIN_MAX
} AdcPinName_t;

#endif
