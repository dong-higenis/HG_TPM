// hipot.h
#ifndef HIPOT_H_
#define HIPOT_H_


#include "ap_def.h"

typedef enum
{
  ID_HIPOT_ACTION = 0x101,  
  ID_HIPOT_CLEAR  = 0x102,
  ID_HIPOT_STATE  = 0x121,
} HipotMsgId_t;

typedef enum
{
  HIPOT_SET_IDLE = 0,
  HIPOT_SET_START,
  HIPOT_SET_STOP,
} HipotSet_t;

typedef enum
{
  HIPOT_RESULT_NONE = 0,
  HIPOT_RESULT_PASS,
  HIPOT_RESULT_FAIL,
  HIPOT_RESULT_TIMEOUT,
} HipotResult_t;

typedef enum
{
  HIPOT_STATE_IDLE = 0,
  HIPOT_STATE_BUSY,
  HIPOT_STATE_PAUSE,
  HIPOT_STATE_DONE,
} HipotState_t;

void hipotInit(void);
void hipotMain(void);


#endif
