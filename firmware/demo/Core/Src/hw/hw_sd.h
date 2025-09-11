#ifndef HW_SD_H_
#define HW_SD_H_

#include "main.h"
#include "stm32g4xx_hal.h"
#include "diskio.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define bool BYTE

// SD카드 명령어 정의
#define CMD0 (0x40 + 0)   /* GO_IDLE_STATE */
#define CMD8 (0x40 + 8)   /* SEND_IF_COND */
#define CMD12 (0x40 + 12) /* STOP_TRANSMISSION */

    // SPI 설정 (사용자가 main.c에서 정의)

    extern SPI_HandleTypeDef hspi1;
#define HSPI_SDCARD &hspi1
#define SD_CS_PORT SD_CS_GPIO_Port
#define SD_CS_PIN SD_CS_Pin

// 타임아웃 설정 (ms)
#define SPI_TIMEOUT 1000

    // 전역 변수 (extern 선언)
    extern volatile uint16_t timer1, timer2;

    // 함수 프로토타입
    void hwSdInit(void);
    uint8_t hwSdCheckPower(void);
    bool hwSdRxDataBlock(BYTE *buff, UINT len);
    BYTE hwSdSendCmd(BYTE cmd, uint32_t arg);
    DSTATUS hwSdGetStatus(void);
    void hwSdClearStatus(DSTATUS flag);
    uint8_t hwSdGetCardType(void);
    void hwSdSetCardType(uint8_t type);
    void hwSdSelect(void);
    void hwSdDeselect(void);
    uint8_t hwSdRxByte(void);
    void hwSdSetTimer(uint16_t ms);
    uint16_t hwSdGetTimer(void);
    uint8_t hwSdReadyWait(void);
    void hwSdPowerOff(void);

#if _USE_WRITE == 1
    bool hwSdTxDataBlock(const uint8_t *buff, BYTE token);
#endif

#ifdef __cplusplus
}
#endif

#endif /* HW_SD_H_ */
