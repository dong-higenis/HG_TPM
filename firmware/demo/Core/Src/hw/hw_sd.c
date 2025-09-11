#include "hw_sd.h"

// 전역 변수 정의
volatile uint16_t timer1, timer2;          /* 1ms 타이머 카운터 */
static volatile DSTATUS stat = STA_NOINIT; /* Disk Status */
static uint8_t cardType;                   /* Type 0:MMC, 1:SDC, 2:Block addressing */
static uint8_t powerFlag = 0;              /* Power flag */

/* 슬레이브 선택 */
static void select(void)
{
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
}

/* 슬레이브 선택 해제 */
static void deselect(void)
{
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
}

/* SPI 데이터 송신 */
static void spiTxByte(uint8_t data)
{
    while (!__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_TXE))
        ;
    HAL_SPI_Transmit(HSPI_SDCARD, &data, 1, SPI_TIMEOUT);
}

/* SPI 전송 버퍼 */
static void spiTxBuffer(uint8_t *buffer, uint16_t len)
{
    while (!__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_TXE))
        ;
    HAL_SPI_Transmit(HSPI_SDCARD, buffer, len, SPI_TIMEOUT);
}

/* SPI 데이터 수신 */
static uint8_t spiRxByte(void)
{
    uint8_t dummy = 0xFF, data;

    while (!__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_TXE))
        ;
    HAL_SPI_TransmitReceive(HSPI_SDCARD, &dummy, &data, 1, SPI_TIMEOUT);

    return data;
}

/* 포인터를 통한 SPI 수신 데이터 */
static void spiRxBytePtr(uint8_t *buff)
{
    *buff = spiRxByte();
}

/* SD카드 준비 상태 확인 */
static uint8_t sdReadyWait(void)
{
    uint8_t res;

    timer2 = 500; // 500ms 타임아웃

    do
    {
        res = spiRxByte();
    } while ((res != 0xFF) && timer2);

    return res;
}

/* 전원 제어 */
static void sdPowerOn(void)
{
    uint8_t args[6];
    uint32_t cnt = 0x1FFF;

    // 웨이크업 시퀀스
    deselect();
    for (int i = 0; i < 10; i++)
    {
        spiTxByte(0xFF);
    }

    select();

    // CMD0 전송
    args[0] = CMD0;
    args[1] = 0;
    args[2] = 0;
    args[3] = 0;
    args[4] = 0;
    args[5] = 0x95;

    spiTxBuffer(args, sizeof(args));

    while ((spiRxByte() != 0x01) && cnt)
    {
        cnt--;
    }

    deselect();
    spiTxByte(0xFF);

    powerFlag = 1;
}

static void sdPowerOff(void)
{
    powerFlag = 0;
}

/***************************************
 * 외부 함수 (public)
 **************************************/

void hwSdInit(void)
{
    sdPowerOn();
}

uint8_t hwSdCheckPower(void)
{
    return powerFlag;
}

void hwSdPowerOff(void)
{
    sdPowerOff();
}

DSTATUS hwSdGetStatus(void)
{
    return stat;
}

void hwSdClearStatus(DSTATUS flag)
{
    stat &= ~flag;
}

uint8_t hwSdGetCardType(void)
{
    return cardType;
}

void hwSdSetCardType(uint8_t type)
{
    cardType = type;
}

void hwSdSelect(void)
{
    select();
}

void hwSdDeselect(void)
{
    deselect();
}

uint8_t hwSdRxByte(void)
{
    return spiRxByte();
}

void hwSdSetTimer(uint16_t ms)
{
    timer1 = ms;
}

uint16_t hwSdGetTimer(void)
{
    return timer1;
}

uint8_t hwSdReadyWait(void)
{
    return sdReadyWait();
}

bool hwSdRxDataBlock(BYTE *buff, UINT len)
{
    uint8_t token;

    timer1 = 200; // 200ms 타임아웃

    // 데이터 토큰 대기
    do
    {
        token = spiRxByte();
    } while ((token == 0xFF) && timer1);

    // 토큰 검증
    if (token != 0xFE)
        return 0;

    // 실제 데이터 수신
    while (len--)
    {
        spiRxBytePtr(buff++);
    }

    // CRC 무시
    spiRxByte();
    spiRxByte();

    return 1;
}

#if _USE_WRITE == 1
bool hwSdTxDataBlock(const uint8_t *buff, BYTE token)
{
    uint8_t resp = 0xFF;
    uint8_t i = 0;

    if (sdReadyWait() != 0xFF)
        return 0;

    spiTxByte(token);

    if (token != 0xFD)
    {
        spiTxBuffer((uint8_t *)buff, 512);

        spiRxByte(); // 더미 CRC 상위
        spiRxByte(); // 더미 CRC 하위

        while (i <= 64)
        {
            resp = spiRxByte();
            if ((resp & 0x1F) == 0x05)
                break;
            i++;
        }

        timer1 = 200;
        while ((spiRxByte() == 0) && timer1)
            ;
    }

    if ((resp & 0x1F) == 0x05)
        return 1;
    return 0;
}
#endif

BYTE hwSdSendCmd(BYTE cmd, uint32_t arg)
{
    uint8_t crc, res;

    if (sdReadyWait() != 0xFF)
        return 0xFF;

    // 명령어 패킷 전송
    spiTxByte(cmd);
    spiTxByte((uint8_t)(arg >> 24));
    spiTxByte((uint8_t)(arg >> 16));
    spiTxByte((uint8_t)(arg >> 8));
    spiTxByte((uint8_t)arg);

    // CRC 계산
    if (cmd == CMD0)
        crc = 0x95;
    else if (cmd == CMD8)
        crc = 0x87;
    else
        crc = 1;

    spiTxByte(crc);

    if (cmd == CMD12)
        spiRxByte();

    // R1 응답 수신
    uint8_t n = 10;
    do
    {
        res = spiRxByte();
    } while ((res & 0x80) && --n);

    return res;
}
