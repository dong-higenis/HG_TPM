#ifndef SD_FUNC_H_
#define SD_FUNC_H_

#include "main.h"
#include "stm32g4xx_hal.h"
#include "ap_sd.h"
#include "ff.h"
#include "ap_oled.h" // OLED 함수용

#ifdef __cplusplus
extern "C"
{
#endif

#define sd_detect_port SD_CD_GPIO_Port
#define sd_detect_pin SD_CD_Pin

  // 글씨 밝기 enum
  enum bright
  {
    BRIGHT_OFF = 0,
    BRIGHT_LOW = 1,
    BRIGHT_ON = 15
  };

  // 전역 변수 extern 선언 (정의는 .c 파일에서)
  extern FATFS fs;
  extern FATFS *p_fs;
  extern FIL fil;
  extern FRESULT fres;
  extern DWORD fre_clust;
  extern UINT bw;
  extern UINT br;
  extern int8_t closeFlag;
  extern uint8_t previousCardState;
  extern char buffer[100];

  // 함수 프로토타입
  DSTATUS sdCardInit(void);
  FRESULT sdCardMount(void);
  FRESULT sdCardUnMount(void);
  void sdIsCardDetected(void);
  void openFile(char *p_fileName);
  void closeFile(void);
  void checkSize(void);

#ifdef __cplusplus
}
#endif

#endif /* SD_FUNC_H_ */
