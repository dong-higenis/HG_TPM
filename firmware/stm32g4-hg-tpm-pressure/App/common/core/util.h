#ifndef UTIL_H_
#define UTIL_H_


#ifdef __cplusplus
 extern "C" {
#endif


#include "def.h"


uint32_t utilConvert8ToU32 (uint8_t *p_data);
uint16_t utilConvert8ToU16 (uint8_t *p_data);

void utilUpdateCrc(uint16_t *p_crc_cur, uint8_t data_in);
uint16_t utilCalcCRC(uint16_t crc_cur, uint8_t *p_data, uint32_t length);
int hex2int(char c);
#ifdef __cplusplus
}
#endif


#endif 