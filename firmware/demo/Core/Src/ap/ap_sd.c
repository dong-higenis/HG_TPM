#include "ap_sd.h"

/***************************************
 * FatFs diskio 함수 구현
 **************************************/

/* SD 초기화 */
DSTATUS sdDiskInitialize(BYTE drv)
{
    uint8_t n, type, ocr[4];

    if (drv)
    {
        return STA_NOINIT;
    }

    if (hwSdGetStatus() & STA_NODISK)
    {
        return hwSdGetStatus();
    }

    hwSdInit();

    // 4단계: 카드 선택
    hwSdSelect();

    type = 0; // 카드 타입 초기화

    // 5단계: IDLE 상태 확인
    if (hwSdSendCmd(CMD0, 0) == 1) // R1 = 0x01 (IDLE 상태) 기대
    {
        hwSdSetTimer(1000); // 1초 타임아웃

        // 6단계: SD v2+ 확인 (CMD8 지원 여부)
        if (hwSdSendCmd(CMD8, 0x1AA) == 1) // 0x1AA = 2.7-3.6V, 0xAA 체크패턴
        {
            // OCR(Operation Conditions Register) 수신
            for (n = 0; n < 4; n++)
            {
                ocr[n] = hwSdRxByte();
            }

            // 전압 범위 및 체크 패턴 확인
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) // 지원 전압 + 체크패턴
            {
                // ACMD41로 초기화 (HCS 비트 설정)
                do
                {
                    if (hwSdSendCmd(CMD55, 0) <= 1 &&
                        hwSdSendCmd(CMD41, 1UL << 30) == 0) // HCS=1 (고용량 지원)
                        break;
                } while (hwSdGetTimer());

                // 초기화 완료 후 OCR 읽기
                if (hwSdGetTimer() && hwSdSendCmd(CMD58, 0) == 0)
                {
                    for (n = 0; n < 4; n++)
                    {
                        ocr[n] = hwSdRxByte();
                    }

                    // CCS 비트 확인 (카드 용량 구조)
                    type = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;
                }
            }
        }
        else
        {
            // SD v1 또는 MMC 카드
            type = (hwSdSendCmd(CMD55, 0) <= 1 && hwSdSendCmd(CMD41, 0) <= 1)
                       ? CT_SD1
                       : CT_MMC;

            do
            {
                if (type == CT_SD1)
                {
                    // SD v1: ACMD41
                    if (hwSdSendCmd(CMD55, 0) <= 1 && hwSdSendCmd(CMD41, 0) == 0)
                        break;
                }
                else
                {
                    // MMC: CMD1
                    if (hwSdSendCmd(CMD1, 0) == 0)
                        break;
                }
            } while (hwSdGetTimer());

            // 블록 크기 설정 (512바이트)
            if (!hwSdGetTimer() || hwSdSendCmd(CMD16, 512) != 0)
            {
                type = 0;
            }
        }
    }

    hwSdSetCardType(type); // 감지된 카드 타입 저장

    hwSdDeselect(); // 카드 비활성화
    hwSdRxByte();   // 추가 클럭

    // 상태 업데이트
    if (type)
    {
        hwSdClearStatus(STA_NOINIT); // 초기화 완료 플래그 제거
    }
    else
    {
        hwSdPowerOff(); // 초기화 실패 시 전원 끄기
    }

    return hwSdGetStatus();
}

/* SD 상태 리턴함수 */
DSTATUS sdDiskStatus(BYTE drv)
{
    if (drv)
        return STA_NOINIT;
    return hwSdGetStatus();
}

/* 섹터 읽기 함수 */
DRESULT sdDiskRead(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    // 1단계: 매개변수 검증
    if (pdrv || !count)
    {
        return RES_PARERR;
    }

    // 2단계: 초기화 상태 확인
    if (hwSdGetStatus() & STA_NOINIT)
    {
        return RES_NOTRDY;
    }

    // 3단계: 주소 변환 (바이트 주소 카드의 경우)
    if (!(hwSdGetCardType() & CT_BLOCK))
        sector *= 512;

    hwSdSelect();

    if (count == 1)
    {
        // 단일 블록 읽기
        if ((hwSdSendCmd(CMD17, sector) == 0) && hwSdRxDataBlock(buff, 512))
            count = 0; // 성공 시 count를 0으로
    }
    else
    {
        // 다중 블록 읽기
        if (hwSdSendCmd(CMD18, sector) == 0)
        {
            do
            {
                if (!hwSdRxDataBlock(buff, 512))
                    break;
                buff += 512; // 다음 블록 버퍼 위치
            } while (--count);

            hwSdSendCmd(CMD12, 0); // 전송 중지 명령
        }
    }

    hwSdDeselect();
    hwSdRxByte();

    return count ? RES_ERROR : RES_OK; // count가 0이면 성공
}

/* 섹터 쓰기 함수 */
#if _USE_WRITE == 1
DRESULT sdDiskWrite(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    // 1단계: 매개변수 검증
    if (pdrv || !count)
        return RES_PARERR;

    // 2단계: 초기화 상태 확인
    if (hwSdGetStatus() & STA_NOINIT)
        return RES_NOTRDY;

    // 3단계: 쓰기 보호 확인
    if (hwSdGetStatus() & STA_PROTECT)
        return RES_WRPRT;

    // 4단계: 주소 변환 (바이트 주소 카드의 경우)
    if (!(hwSdGetCardType() & CT_BLOCK))
        sector *= 512;

    hwSdSelect();

    if (count == 1)
    {
        // 단일 블록 쓰기
        if ((hwSdSendCmd(CMD24, sector) == 0) &&
            hwSdTxDataBlock(buff, TOKEN_START_BLOCK))
            count = 0; // 성공 시 count를 0으로
    }
    else
    {
        // 다중 블록 쓰기
        if (hwSdGetCardType() & CT_SD1)
        {
            hwSdSendCmd(CMD55, 0);
            hwSdSendCmd(CMD23, count); /* ACMD23 */
        }

        if (hwSdSendCmd(CMD25, sector) == 0)
        {
            do
            {
                if (!hwSdTxDataBlock(buff, TOKEN_MULTI_WRITE))
                    break;
                buff += 512; // 다음 블록 버퍼 위치
            } while (--count);

            /* STOP_TRAN 토큰 */
            if (!hwSdTxDataBlock(0, TOKEN_STOP_TRAN))
            {
                count = 1;
            }
        }
    }

    hwSdDeselect();
    hwSdRxByte();

    return count ? RES_ERROR : RES_OK; // count가 0이면 성공
}
#endif /* _USE_WRITE */

/* ioctl */
DRESULT sdDiskIoctl(BYTE drv, BYTE ctrl, void *buff)
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
            hwSdPowerOff();
            res = RES_OK;
            break; // 전원 끄기
        case 1:
            hwSdInit();
            res = RES_OK;
            break; // 전원 켜기
        case 2:
            *(ptr + 1) = hwSdCheckPower();
            res = RES_OK;
            break; // 전원 상태 확인
        default:
            res = RES_PARERR;
        }
    }
    else
    {
        if (hwSdGetStatus() & STA_NOINIT)
            return RES_NOTRDY;

        hwSdSelect();

        switch (ctrl)
        {
        case GET_SECTOR_COUNT: // 총 섹터 수 얻기
            if ((hwSdSendCmd(CMD9, 0) == 0) && hwSdRxDataBlock(csd, 16))
            {
                if ((csd[0] >> 6) == 1) /* SDC V2 */
                {
                    // SDv2 CSD 파싱
                    DWORD cSize;
                    cSize = (DWORD)(csd[7] & 0x3F) << 16 | (WORD)csd[8] << 8 | csd[9];
                    *(DWORD *)buff = (cSize + 1) << 10; // (C_SIZE+1) * 1024
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
            if (hwSdReadyWait() == 0xFF)
                res = RES_OK;
            break;

        case GET_BLOCK_SIZE: // 블록 크기 (SD카드는 보통 1)
            *(DWORD *)buff = 1;
            res = RES_OK;
            break;
        }

        hwSdDeselect();
        hwSdRxByte();
    }

    return res;
}
