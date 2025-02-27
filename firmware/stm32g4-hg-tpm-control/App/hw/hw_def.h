#ifndef HW_DEF_H_
#define HW_DEF_H_


#include "bsp.h"


#define _DEF_FIRMWATRE_VERSION "V241227R1"
#define _DEF_BOARD_NAME        "STM32G4-CGM-TEST-OLED"


#define _USE_HW_LED
#define HW_LED_MAX_CH 2

#define _USE_HW_UART
#define HW_UART_MAX_CH   3
#define HW_UART_CH_DEBUG _DEF_UART1
#define HW_UART_CH_RS232 _DEF_UART2
#define HW_UART_CH_USB   _DEF_UART3

#define _USE_HW_LOG
#define HW_LOG_CH           HW_UART_CH_DEBUG
#define HW_LOG_BOOT_BUF_MAX 512 // 2048
#define HW_LOG_LIST_BUF_MAX 512 // 4096

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

#define _USE_HW_SSD1322
#define HW_SSD1322_MAX_CH 1

#define _USE_HW_GPIO
#define HW_GPIO_MAX_CH 15

#define _USE_HW_SPI
#define HW_SPI_MAX_CH 1

#define _USE_HW_LCD
#define HW_LCD_WIDTH  256
#define HW_LCD_HEIGHT 64

#define _USE_HW_BUTTON
#define HW_BUTTON_MAX_CH 4
#define HW_BUTTON_CH_S1  _DEF_BUTTON1
#define HW_BUTTON_CH_S2  _DEF_BUTTON2
#define HW_BUTTON_CH_S3  _DEF_BUTTON3
#define HW_BUTTON_CH_S4  _DEF_BUTTON4

#endif
