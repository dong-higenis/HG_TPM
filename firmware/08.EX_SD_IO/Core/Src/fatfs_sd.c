#define TRUE 1
#define FALSE 0
#define bool BYTE

#include "stm32g4xx_hal.h"

#include "diskio.h"
#include "fatfs_sd.h"

// 인터럽트에 의해 값이 변경되는 변수는 컴파일러 최적화로 인한 오작동을 막기 위해 volatile로 선언해야 합니다.
volatile uint16_t Timer1; /* 1ms 타이머 카운터 */
volatile uint16_t Timer2;

static volatile DSTATUS stat = STA_NOINIT; /* Disk Status */
static uint8_t card_type;                  /* Type 0:MMC, 1:SDC, 2:Block addressing */
static uint8_t power_Flag = 0;             /* Power flag */

/*
 Timer1, Timer2: 타임아웃 카운터 (1ms 단위, 인터럽트에서 감소)
 Stat: 디스크 상태 (STA_NOINIT=초기화안됨, STA_NODISK=디스크없음)
 CardType: 감지된 카드 타입 저장
 PowerFlag: 전원 상태 플래그
 */

/**
 * @brief SPI 함수들 ( SPI 신호 동작관련 )
 */

// 슬레이브 선택!
static void SELECT(void)
{
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET); // CS를 LOW로
    HAL_Delay(1);                                             // 1ms 대기 (안정화)
}

// 슬레이브 선택 해제!
static void DESELECT(void)
{
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET); // CS를 HIGH로
    HAL_Delay(1);                                           // 1ms 대기 (안정화)
}

// SPI 데이터 송신!
static void SPI_TxByte(uint8_t data)
{
    while (!__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_TXE))
        ;                                                 // TX 버퍼 비울 때까지 대기
    HAL_SPI_Transmit(HSPI_SDCARD, &data, 1, SPI_TIMEOUT); // 1바이트 전송
}

// SPI 전송 버퍼!
static void SPI_TxBuffer(uint8_t *buffer, uint16_t len)
{
    while (!__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_TXE))
        ;
    HAL_SPI_Transmit(HSPI_SDCARD, buffer, len, SPI_TIMEOUT);
}

// SPI 데이터 수신!
static uint8_t SPI_RxByte(void)
{
    uint8_t dummy, data;
    dummy = 0xFF; // SD카드는 0xFF를 보내야 응답함

    while (!__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_TXE))
        ;
    HAL_SPI_TransmitReceive(HSPI_SDCARD, &dummy, &data, 1, SPI_TIMEOUT);

    return data;
}

// 포인터를 통한 SPI 수신 데이터
static void SPI_RxBytePtr(uint8_t *buff)
{
    *buff = SPI_RxByte();
}

/**
 * @brief SD카드 관련 함수
 */
// SD카드 준비 상태 확인
static uint8_t SD_ReadyWait(void)
{
    uint8_t res;

    // 500ms 타임아웃 설정
    Timer2 = 500;

    do
    {
        res = SPI_RxByte();
    } while ((res != 0xFF) && Timer2);

    return res;
}

// 전원 제어
static void SD_PowerOn(void)
{
    uint8_t args[6];
    uint32_t cnt = 0x1FFF; // 약 8000번 시도

    /* 1단계: 웨이크업 시퀀스 */
    DESELECT(); // 슬레이브 선택 해제!
    for (int i = 0; i < 10; i++)
    {
        SPI_TxByte(0xFF); // 80클럭 펄스 생성 (10바이트 × 8비트)
    }

    /* 슬레이브 선택! */
    SELECT();

    /* 2단계: 카드 선택 및 IDLE 명령 */
    args[0] = CMD0; // 명령어: GO_IDLE_STATE
    args[1] = 0;    // 인수 [31:24] = 0
    args[2] = 0;    // 인수 [23:16] = 0
    args[3] = 0;    // 인수 [15:8] = 0
    args[4] = 0;    // 인수 [7:0] = 0
    args[5] = 0x95; // CRC (CMD0용 고정값)

    SPI_TxBuffer(args, sizeof(args)); // 6바이트 전송

    /* 3단계: 응답 대기 (R1 응답 = 0x01 기대) */
    while ((SPI_RxByte() != 0x01) && cnt)
    {
        cnt--; // 타임아웃 카운터
    }

    DESELECT();       // 명령 완료 후 비활성화
    SPI_TxByte(0XFF); // 추가 클럭 제공

    PowerFlag = 1; // 전원 상태 플래그 설정
}

/* power off */
static void SD_PowerOff(void)
{
    PowerFlag = 0;
}

/* Power Flag 체크용 함수! */
static uint8_t SD_CheckPower(void)
{
    return PowerFlag;
}

/* 데이터 블록 수신 */
static bool SD_RxDataBlock(BYTE *buff, UINT len)
{
    uint8_t token;

    /* 200ms 타임아웃 */
    Timer1 = 200;

    /* 응답을 받을때 까지 or 타임아웃 시간까지 루프를 돕니다. */
    // 1단계: 데이터 토큰 대기
    do
    {
        token = SPI_RxByte();
    } while ((token == 0xFF) && Timer1);

    // 2단계: 토큰 검증
    if (token != 0xFE)
        return FALSE;

    // 3단계: 실제 데이터 수신
    while (len--)
    {
        SPI_RxBytePtr(buff++);
    }

    // 4단계: CRC 무시 (사용 안 함)
    SPI_RxByte();
    SPI_RxByte();

    return TRUE;
}

/*
 *
 * [0xFF 대기] → [0xFE 토큰] → [512바이트 데이터] → [2바이트 CRC]
 */

/* 데이터 블록 전송 */
#if _USE_WRITE == 1
static bool SD_TxDataBlock(const uint8_t *buff, BYTE token)
{
    uint8_t resp = 0xFF; // 초기화
    uint8_t i = 0;

    // 1단계: 카드 준비 상태 확인
    if (SD_ReadyWait() != 0xFF)
        return FALSE;

    // 2단계: 토큰 전송
    SPI_TxByte(token); // 0xFE(단일블록) 또는 0xFC(다중블록)

    // 3단계: 데이터 전송 (STOP 토큰이 아닌 경우만)
    if (token != 0xFD) // 0xFD = STOP_TRANSMISSION 토큰
    {
        SPI_TxBuffer((uint8_t *)buff, 512); // 512바이트 데이터

        SPI_RxByte(); // 더미 CRC 상위
        SPI_RxByte(); // 더미 CRC 하위

        // 4단계: 데이터 응답 토큰 수신
        while (i <= 64)
        {
            resp = SPI_RxByte();

            if ((resp & 0x1F) == 0x05)
                break; // 0x05 = 데이터 수락됨
            i++;
        }

        // 5단계: 바쁨 상태 대기
        Timer1 = 200; // 200ms 타임아웃
        while ((SPI_RxByte() == 0) && Timer1)
            ;
    }
    if ((resp & 0x1F) == 0x05)
        return TRUE;

    return FALSE;
}
#endif /* _USE_WRITE */

// 명령어 전송 함수!
static BYTE SD_SendCmd(BYTE cmd, uint32_t arg)
{
    uint8_t crc, res;

    // 1단계: 카드 준비 대기
    if (SD_ReadyWait() != 0xFF)
    {
        return 0xFF;
    }

    // 2단계: 명령어 패킷 전송 (6바이트)
    SPI_TxByte(cmd);                  // 명령어 바이트
    SPI_TxByte((uint8_t)(arg >> 24)); // 인수 [31:24]
    SPI_TxByte((uint8_t)(arg >> 16)); // 인수 [23:16]
    SPI_TxByte((uint8_t)(arg >> 8));  // 인수 [15:8]
    SPI_TxByte((uint8_t)arg);         // 인수 [7:0]

    // 3단계: CRC 계산 및 전송
    if (cmd == CMD0)
    {
        crc = 0x95; // CMD0은 항상 0x95
    }
    else if (cmd == CMD8)
    {
        crc = 0x87; // CMD8(0x1AA)는 0x87
    }
    else
    {
        crc = 1; // 나머지는 더미 CRC
    }
    SPI_TxByte(crc);

    // 4단계: CMD12 특수 처리
    if (cmd == CMD12)
    {
        SPI_RxByte(); // STOP_TRANSMISSION 후 더미바이트
    }

    // 5단계: R1 응답 수신 (최대 10번 시도)
    uint8_t n = 10;
    do
    {
        res = SPI_RxByte();
    } while ((res & 0x80) && --n); // MSB가 0이 될 때까지

    return res;
}

/**
 * @brief 사용자 정의 diskio.c 함수!
 */
/* SD 초기화 */
DSTATUS SD_disk_initialize(BYTE drv)
{
    uint8_t n, type, ocr[4];

    // 1단계: 드라이브 번호 확인 (0만 지원)
    if (drv)
        return STA_NOINIT;

    // 2단계: 물리적 디스크 존재 확인
    if (Stat & STA_NODISK)
        return Stat;

    // 3단계: 전원 켜기
    SD_PowerOn();

    // 4단계: 카드 선택
    SELECT();

    type = 0; // 카드 타입 초기화

    // 5단계: IDLE 상태 확인
    if (SD_SendCmd(CMD0, 0) == 1) // R1 = 0x01 (IDLE 상태) 기대
    {
        Timer1 = 1000; // 1초 타임아웃

        // 6단계: SD v2+ 확인 (CMD8 지원 여부)
        if (SD_SendCmd(CMD8, 0x1AA) == 1) // 0x1AA = 2.7-3.6V, 0xAA 체크패턴
        {
            // OCR(Operation Conditions Register) 수신
            for (n = 0; n < 4; n++)
            {
                ocr[n] = SPI_RxByte();
            }

            // 전압 범위 및 체크 패턴 확인
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) // 지원 전압 + 체크패턴
            {
                // ACMD41로 초기화 (HCS 비트 설정)
                do
                {
                    if (SD_SendCmd(CMD55, 0) <= 1 &&
                        SD_SendCmd(CMD41, 1UL << 30) == 0) // HCS=1 (고용량 지원)
                        break;
                } while (Timer1);

                // 초기화 완료 후 OCR 읽기
                if (Timer1 && SD_SendCmd(CMD58, 0) == 0)
                {
                    for (n = 0; n < 4; n++)
                    {
                        ocr[n] = SPI_RxByte();
                    }

                    // CCS 비트 확인 (카드 용량 구조)
                    type = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;
                }
            }
        }
        else
        {
            // SD v1 또는 MMC 카드
            type = (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(CMD41, 0) <= 1)
                       ? CT_SD1
                       : CT_MMC;

            do
            {
                if (type == CT_SD1)
                {
                    // SD v1: ACMD41
                    if (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(CMD41, 0) == 0)
                        break;
                }
                else
                {
                    // MMC: CMD1
                    if (SD_SendCmd(CMD1, 0) == 0)
                        break;
                }
            } while (Timer1);

            // 블록 크기 설정 (512바이트)
            if (!Timer1 || SD_SendCmd(CMD16, 512) != 0)
                type = 0;
        }
    }

    CardType = type; // 감지된 카드 타입 저장

    DESELECT();   // 카드 비활성화
    SPI_RxByte(); // 추가 클럭

    // 상태 업데이트
    if (type)
    {
        Stat &= ~STA_NOINIT; // 초기화 완료 플래그 제거
    }
    else
    {
        SD_PowerOff(); // 초기화 실패 시 전원 끄기
    }

    return Stat;
}

/* SD 상태 리턴함수! */
DSTATUS SD_disk_status(BYTE drv)
{
    if (drv)
        return STA_NOINIT;
    return Stat;
}

/* 섹터 읽기 함수! */
DRESULT SD_disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    // 1단계: 매개변수 검증
    if (pdrv || !count)
        return RES_PARERR;

    // 2단계: 초기화 상태 확인
    if (Stat & STA_NOINIT)
        return RES_NOTRDY;

    // 3단계: 주소 변환 (바이트 주소 카드의 경우)
    if (!(CardType & CT_BLOCK))
        sector *= 512;

    SELECT();

    if (count == 1)
    {
        // 단일 블록 읽기
        if ((SD_SendCmd(CMD17, sector) == 0) && SD_RxDataBlock(buff, 512))
            count = 0; // 성공 시 count를 0으로
    }
    else
    {
        // 다중 블록 읽기
        if (SD_SendCmd(CMD18, sector) == 0)
        {
            do
            {
                if (!SD_RxDataBlock(buff, 512))
                    break;
                buff += 512; // 다음 블록 버퍼 위치
            } while (--count);

            SD_SendCmd(CMD12, 0); // 전송 중지 명령
        }
    }

    DESELECT();
    SPI_RxByte();

    return count ? RES_ERROR : RES_OK; // count가 0이면 성공
}
/* 섹터 쓰기 함수! */
#if _USE_WRITE == 1
DRESULT SD_disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    // 1단계: 매개변수 검증
    if (pdrv || !count)
        return RES_PARERR;

    // 2단계: 초기화 상태 확인
    if (Stat & STA_NOINIT)
        return RES_NOTRDY;

    // 3단계: 쓰기 보호 확인
    if (Stat & STA_PROTECT)
        return RES_WRPRT;

    // 4단계: 주소 변환 (바이트 주소 카드의 경우)
    if (!(CardType & CT_BLOCK))
        sector *= 512;

    SELECT();

    if (count == 1)
    {
        // 단일 블록 쓰기
        if ((SD_SendCmd(CMD24, sector) == 0) && SD_TxDataBlock(buff, 0xFE))
            count = 0; // 성공 시 count를 0으로
    }
    else
    {
        // 다중 블록 쓰기
        if (CardType & CT_SD1)
        {
            SD_SendCmd(CMD55, 0);
            SD_SendCmd(CMD23, count); /* ACMD23 */
        }

        if (SD_SendCmd(CMD25, sector) == 0)
        {
            do
            {
                if (!SD_TxDataBlock(buff, 0xFC))
                    break;
                buff += 512; // 다음 블록 버퍼 위치
            } while (--count);

            /* STOP_TRAN 토큰 */
            if (!SD_TxDataBlock(0, 0xFD))
            {
                count = 1;
            }
        }
    }

    DESELECT();
    SPI_RxByte();

    return count ? RES_ERROR : RES_OK; // count가 0이면 성공
}
#endif /* _USE_WRITE */

/* ioctl */
DRESULT SD_disk_ioctl(BYTE drv, BYTE ctrl, void *buff)
{
    DRESULT res;
    uint8_t n, csd[16], *ptr = buff;

    if (drv)
        return RES_PARERR;
    res = RES_ERROR;

    if (ctrl == CTRL_POWER) // 전원 제어
    {
        switch (*ptr)
        {
        case 0:
            SD_PowerOff();
            res = RES_OK;
            break; // 전원 끄기
        case 1:
            SD_PowerOn();
            res = RES_OK;
            break; // 전원 켜기
        case 2:
            *(ptr + 1) = SD_CheckPower();
            res = RES_OK;
            break; // 전원 상태 확인
        default:
            res = RES_PARERR;
        }
    }
    else
    {
        if (Stat & STA_NOINIT)
            return RES_NOTRDY;

        SELECT();

        switch (ctrl)
        {
        case GET_SECTOR_COUNT: // 총 섹터 수 얻기
            if ((SD_SendCmd(CMD9, 0) == 0) && SD_RxDataBlock(csd, 16))
            {
                if ((csd[0] >> 6) == 1) /* SDC V2 */
                {
                    DWORD c_size;
                    c_size = (DWORD)(csd[7] & 0x3F) << 16 | (WORD)csd[8] << 8 | csd[9];
                    *(DWORD *)buff = (c_size + 1) << 10; // (C_SIZE+1) * 1024
                }
                else /* MMC or SDC V1 */
                {
                    // 복잡한 CSD v1 계산
                    WORD csize;
                    n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
                    csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
                    *(DWORD *)buff = (DWORD)csize << (n - 9);
                }
                res = RES_OK;
            }
            break;

        case GET_SECTOR_SIZE: // 섹터 크기 (항상 512)
            *(WORD *)buff = 512;
            res = RES_OK;
            break;

        case CTRL_SYNC: // 동기화 (쓰기 완료 대기)
            if (SD_ReadyWait() == 0xFF)
                res = RES_OK;
            break;

            // 추가 정보들...
        }

        DESELECT();
        SPI_RxByte();
    }

    return res;
}
