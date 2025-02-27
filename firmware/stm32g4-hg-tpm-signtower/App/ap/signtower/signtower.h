#ifndef SIGNTOWER_H_
#define SIGNTOWER_H_

#include "ap_def.h"

typedef enum
{
  ID_SIGNTOWER_LED_SET = 0x104,
  ID_SIGNTOWER_BUZZER_SET = 0x105,
  ID_SIGNTOWER_STATE = 0x141,
} SignTowerMsgId_t;

typedef enum
{
  SIGNTOWER_COLOR_OFF = 0,
  SIGNTOWER_COLOR_RED,
  SIGNTOWER_COLOR_YELLOW,
  SIGNTOWER_COLOR_GREEN,
} SignTowerColor_t;

void signtowerInit(void);
void signtowerMain(void);

#endif