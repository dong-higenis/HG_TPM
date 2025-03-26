#ifndef FRAM_H_
#define FRAM_H_

#include "hw_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _USE_HW_FRAM



bool     framInit();
bool     framIsInit(void);
bool     framValid(uint32_t addr);
bool     framReadByte(uint32_t addr, uint8_t *p_data);
bool     framWriteByte(uint32_t addr, uint8_t data_in);
bool     framRead(uint32_t addr, uint8_t *p_data, uint32_t length);
bool     framWrite(uint32_t addr, uint8_t *p_data, uint32_t length);

uint32_t framGetLength(void);
bool     framFormat(void);

#endif

#ifdef __cplusplus
}
#endif

#endif /* FRAM_H_ */
