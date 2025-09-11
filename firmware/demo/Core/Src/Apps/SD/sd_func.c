

#include "sd_func.h"

// ✨ 이 부분이 누락되어서 에러 발생!
// 전역 변수 정의 (반드시 추가해야 함)
FATFS fs;
FATFS *p_fs;
FIL fil;
FRESULT fres;
DWORD fre_clust;
UINT bw;
UINT br;
int8_t closeFlag = 1;
uint8_t previousCardState = 0;
char buffer[100];

DSTATUS sdCardInit(void)
{
  DSTATUS result = sdDiskInitialize(0);
  if (result == 0)
  {
    printf("sdcard inited\n");
  }
  else
  {
    printf("sdcard initiation failed: %d\n", result);
  }

  return result;
}

FRESULT sdCardMount(void)
{
  fres = f_mount(&fs, "", 0);
  if (fres == FR_OK)
  {
    printf("sdcard mounte!\n");
  }
  else
  {
    printf("sdcard mount failed: %d\n", fres);
  }
  return fres;
}

FRESULT sdCardUnMount(void)
{
  fres = f_mount(NULL, "", 0);
  if (fres == FR_OK)
  {
    printf("sdcard Un-mounted\n");
  }
  else
  {
    printf("sdcard Un-mount failed: %d\n", fres);
  }
  return fres;
}

void sdIsCardDetected(void)
{
  uint8_t currentCardState = (HAL_GPIO_ReadPin(sd_detect_port, sd_detect_pin) == GPIO_PIN_RESET);
  if (currentCardState && !previousCardState)
  {
    HAL_Delay(200);
    sdCardMount();
    eyesSurprisedAnimation();
  }
  else if (!currentCardState && previousCardState)
  {
    sdCardUnMount();
    eyesSadAnimation();
  }
  previousCardState = currentCardState;
}

// 파일 열기(없으면 생성). append 모드로 사용
void openFile(char *p_fileName)
{
  if (closeFlag == 0)
  {
    printf("File already open! Close it first.\r\n");
    return;
  }

  // 파일이 있으면 열고, 없으면 생성하여 append 모드로 사용
  fres = f_open(&fil, p_fileName, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);

  if (fres == FR_OK)
  {
    // 항상 파일 끝으로 이동하여 이어쓰기(Append)
    f_lseek(&fil, f_size(&fil));

    printf("File '%s' ready for writing!\r\n", p_fileName);
    if (f_size(&fil) > 0)
    {
      printf("File size: %lu bytes\r\n", f_size(&fil));
    }
    else
    {
      printf("New file created.\r\n");
    }
  }
  else
  {
    printf("Failed to open/create file '%s'. Error: %d\r\n", p_fileName, fres);
    return;
  }
  closeFlag = 0; // 이제 열림 상태
}

// 열린 파일 닫기
void closeFile(void)
{
  fres = f_close(&fil);
  if (fres == FR_OK)
  {
    printf("File Closed !\r\n");
  }
  else if (fres != FR_OK)
  {
    printf("File Close Failed... \r\n");
  }
  closeFlag = 1;
}

// SD 카드 여유 공간 조회 (KB 단위로 표시)
void checkSize(void)
{
  fres = f_getfree("", &fre_clust, &p_fs);
  if (fres == FR_OK)
  {
    // 클러스터 수 * 클러스터당 섹터수 * 섹터당 512바이트 → KB로 환산( /1024 = *0.5)
    uint32_t freeKB = (uint32_t)(fre_clust * p_fs->csize * 0.5f);
    uint32_t totalKB = (uint32_t)((p_fs->n_fatent - 2) * p_fs->csize * 0.5f);
    printf("Free: %lu KB / Total: %lu KB\r\n", freeKB, totalKB);
  }
  else
  {
    printf("Failed to get free space. (FRESULT=%d)\r\n", fres);
  }
}

// 현재 열린 파일 끝에 한 줄을 추가로 기록
void writeFile(char *p_text)
{
  if (closeFlag != 0)
  {
    printf("No file is open! Use 'open <filename>' first.\r\n");
    return;
  }

  // 안전하게 개행을 붙여 한 줄 단위로 기록
  snprintf(buffer, sizeof(buffer), "%s\r\n", p_text);

  // 파일 끝으로 이동 (append)
  fres = f_lseek(&fil, f_size(&fil));
  if (fres != FR_OK)
  {
    printf("Can't move to end of file\r\n");
    return;
  }

  sprintf(buffer, "%s\r\n", p_text);
  fres = f_write(&fil, buffer, strlen(buffer), &bw);

  if (fres == FR_OK)
  {
    printf("Writing Complete! %lu bytes written.\r\n", bw);
    f_sync(&fil); // 즉시 저장(전원 차단에 대비)
  }
  else
  {
    printf("Writing Failed\r\n");
  }
}

// 기존의 파일 내용 읽기
void readFile(char *p_fileName)
{
  if (closeFlag == 0)
  {
    closeFile();
  }
  fres = f_open(&fil, p_fileName, FA_READ);
  if (fres == FR_OK)
  {
    printf("File '%s' opened for reading.\r\n", p_fileName);
  }
  else
  {
    printf("Failed to open file '%s' for reading!\r\n", p_fileName);
    return;
  }

  // 파일 전체 읽기
  memset(buffer, 0, sizeof(buffer));
  fres = f_read(&fil, buffer, sizeof(buffer) - 1, &br);

  if (fres == FR_OK && br > 0)
  {
    printf("-----------FILE CONTENT----------\r\n");
    printf("%s", buffer);
    if (buffer[strlen(buffer) - 1] != '\n')
      printf("\r\n");
    printf("-----------END OF FILE-----------\r\n");
    printf("%lu bytes read.\r\n", br);
  }
  else
  {
    printf("File is empty or read failed!\r\n");
  }

  f_close(&fil); // 읽기 후 파일 닫기
}
