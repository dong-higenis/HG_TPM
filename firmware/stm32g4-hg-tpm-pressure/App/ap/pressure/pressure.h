#ifndef PRESSURE_H_
#define PRESSURE_H_

#include "ap_def.h"

typedef enum
{
  ID_PRESSURE_SET = 0x103,
  ID_PRESSURE_STATE = 0x131,
} PressureMsgId_t;

void pressureInit(void);
void pressureMain(void);
#endif