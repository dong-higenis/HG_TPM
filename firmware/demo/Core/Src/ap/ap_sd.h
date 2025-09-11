#ifndef AP_SD_H_
#define AP_SD_H_

#include "stm32g4xx_hal.h"
#include "diskio.h"
#include "hw_sd.h"

#ifdef __cplusplus
extern "C" {
#endif

// 카드 타입 정의
#define CT_MMC     0x01    /* MMC v3 */
#define CT_SD1     0x02    /* SD v1 */
#define CT_SD2     0x04    /* SD v2 */
#define CT_BLOCK   0x08    /* Block addressing */

// CMD 명령어 추가 정의
#define CMD1    (0x40+1)    /* SEND_OP_COND (MMC) */
#define CMD9    (0x40+9)    /* SEND_CSD */
#define CMD16   (0x40+16)   /* SET_BLOCKLEN */
#define CMD17   (0x40+17)   /* read_single_block */
#define CMD18   (0x40+18)   /* read_multiple_block */
#define CMD23   (0x40+23)   /* SET_BLOCK_COUNT (MMC) */
#define CMD24   (0x40+24)   /* WRITE_BLOCK */
#define CMD25   (0x40+25)   /* WRITE_MULTIPLE_BLOCK */
#define CMD41   (0x40+41)   /* SEND_OP_COND (SDC) */
#define CMD55   (0x40+55)   /* APP_CMD */
#define CMD58   (0x40+58)   /* read_ocr */

// 데이터 토큰 정의
#define TOKEN_START_BLOCK    0xFE  /* 데이터 블록 시작 */
#define TOKEN_MULTI_WRITE    0xFC  /* 다중 블록 쓰기 */
#define TOKEN_STOP_TRAN      0xFD  /* 전송 중지 */


// FatFs diskio 함수들
DSTATUS sdDiskInitialize(BYTE drv);
DSTATUS sdDiskStatus(BYTE drv);
DRESULT sdDiskRead(BYTE pdrv, BYTE* buff, DWORD sector, UINT count);

#if _USE_WRITE == 1
DRESULT sdDiskWrite(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
#endif

DRESULT sdDiskIoctl(BYTE drv, BYTE ctrl, void *buff);

#ifdef __cplusplus
}
#endif

#endif /* AP_SD_H_ */
