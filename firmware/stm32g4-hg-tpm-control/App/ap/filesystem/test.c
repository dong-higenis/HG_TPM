/*
 * test.c
 *
 *  Created on: Mar 6, 2025
 *      Author: user
 */

#include "ap_def.h"

uint16_t readFile(char *address, uint8_t *buffer, uint8_t read_size, uint32_t index);

void test_fatfs(void)
{
    uint16_t read_data[10];
    FRESULT res;
    res = readFile("read_me.txt", read_data, 10, 0);
    logPrintf("res=%d\n", res);
    logPrintf("read data=%s\n", read_data);
}

uint16_t readFile(char *address, uint8_t *buffer, uint8_t read_size, uint32_t index)
{
  uint16_t res = 0;
  uint32_t br;
  FSIZE_t file_size;
  FIL fil;
  FRESULT fresult = FR_OK;
  uint16_t readSize = read_size;
  FATFS FatFs;

  f_mount(&FatFs, "", 0);
  fresult = f_open(&fil, (TCHAR *)address, FA_READ);

  if(fresult == FR_OK) {
    // Check the file size
    file_size = f_size(&fil);
    //UARTprintf("file_size= %lBytes\n",(Uint32)file_size);
    // If the file size is smaller than the specified read size
    if((uint16_t)file_size < read_size) {
      readSize = (uint16_t)file_size;
    }

    // Set the file read pointer
    fresult = f_lseek(&fil, (FSIZE_t)index);

    if(fresult == FR_OK) {
      fresult = f_read(&fil, buffer, (uint32_t)readSize, &br);
      if(fresult == FR_OK) {
        f_close(&fil);
      }
    }
  }

  if(fresult == FR_OK) {
    res = br;
  } else {
    res = 0;
  }

  return res;
}
