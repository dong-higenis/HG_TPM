#ifndef US6300_H_
#define US6300_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "hw_def.h"

#ifdef _USE_HW_US6300


typedef struct
{
  float    psi;
  int32_t  psi_x100;
  uint32_t adc;
  float    mBar;
  int32_t  mBar_x100;
  float    mmHg;
} us6300_info_t;


bool us6300Init(void);
bool us6300GetInfo(us6300_info_t *p_info);
bool us6300GetInfoChannel(uint8_t ch, us6300_info_t *p_info);

#endif

#ifdef __cplusplus
}
#endif

#endif