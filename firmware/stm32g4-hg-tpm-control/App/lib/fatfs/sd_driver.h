/*
 * sd_driver.h
 *
 *  Created on: 2024. 4. 28.
 *      Author: User
 */

#ifndef SD_DRIVER_H_
#define SD_DRIVER_H_

void DESELECT (void);
void SELECT (void);
DSTATUS MMC_disk_initialize(void);
DRESULT MMC_disk_write(const BYTE *buff, DWORD sector, UINT count);
DRESULT MMC_disk_read(BYTE *buff, DWORD sector, UINT count);
DRESULT MMC_disk_ioctl ( BYTE ctrl, void *buff );

DSTATUS SD_Init (void);

BYTE SD_WriteSingleBlock (DWORD sector, const BYTE * data);
BYTE SD_WriteMultiBlock (DWORD sector, const BYTE * data, BYTE count);
BYTE SD_ReadSingleBlock (DWORD sector, BYTE * buffer);
BYTE SD_ReadMultiBlock (DWORD sector, BYTE * buffer, BYTE count);

BYTE SD_SendCommand (BYTE cmd, DWORD arg, BYTE crc);
BYTE SD_SendCommand_NoDeassert (BYTE cmd, DWORD arg, BYTE crc);
BYTE bsp_readwritebyte_spi2 (BYTE tx_data);
BYTE SD_ReceiveData (BYTE * data, DWORD len, BYTE release);
BYTE SD_WaitDataReady (void);
BYTE SD_GetResponse (BYTE Response);
BYTE SD_Idle_Sta (void);

BYTE SD_GetCSD (BYTE *csd_data);
DWORD SD_GetCapacity (void);

#endif /* SD_DRIVER_H_ */
