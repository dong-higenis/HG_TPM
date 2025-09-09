#ifndef TCA9548_H
#define TCA9548_H

#include "hw_def.h"

#define TCA9548_MAX_CH HW_TCA9548_MAX_CH

bool tca9548Init(void);
bool tca9548SelectChannel(uint8_t channel);

#endif // TCA9548_H 