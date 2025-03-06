/*
 * sd_driver.c
 *
 *  Created on: 2024. 4. 27.
 *      Author: User
 */

/** @file
 * @brief This file contains the MMC driver implementation for the TMS320F2837x microcontroller.
 */

#include "ap_def.h"

//------------------------------------------------------------------------------------------------------------------------------------------------

// SD data transmission whether to release the bus after the end of the macro definition
#define NO_RELEASE 0 /**< Macro definition for not releasing the SD bus after data transmission */
#define RELEASE 1    /**< Macro definition for releasing the SD bus after data transmission */

// SD Card Type Definition
#define SD_TYPE_MMC 0  /**< SD Card Type: MultiMediaCard (MMC) */
#define SD_TYPE_V1 1   /**< SD Card Type: Version 1.x Standard Capacity (<=2GB) */
#define SD_TYPE_V2 2   /**< SD Card Type: Version 2.0 Standard Capacity (<=2GB) */
#define SD_TYPE_V2HC 4 /**< SD Card Type: Version 2.0 High Capacity (SDHC, >2GB) */

// SD Card instruction sheet
#define CMD0    0   /**< Card Reset */
#define CMD1    1   /**< Command 1 */
#define CMD9    9   /**< Command 9, CSD data read */
#define CMD10   10  /**< Command 10, read CID data */
#define CMD12   12  /**< Command 12, to stop data transmission */
#define CMD16   16  /**< Command 16, set SectorSize should return 0x00 */
#define CMD17   17  /**< Command 17, read sector */
#define CMD18   18  /**< Command 18, read Multi sector */
#define ACMD23  23  /**< Command 23, set the multi-sector erase writing in advance of a block N */
#define CMD24   24  /**< Command 24, write sector */
#define CMD25   25  /**< Command 25, write Multi sector */
#define ACMD41  41  /**< Command 41, should return 0x00 */
#define CMD55   55  /**< Command 55, should return 0x01 */
#define CMD58   58  /**< Command 58, read OCR information */
#define CMD59   59  /**< Command 59, enables/disables the CRC, should return 0x00 */

// Write data to respond to the word meaning
#define MSD_DATA_OK                 0x05    /**< Data write successful */
#define MSD_DATA_CRC_ERROR          0x0B    /**< Data CRC error */
#define MSD_DATA_WRITE_ERROR        0x0D    /**< Data write error */
#define MSD_DATA_OTHER_ERROR        0xFF    /**< Other data error */

// SD card labeled word response
#define MSD_RESPONSE_NO_ERROR       0x00    /**< No error */
#define MSD_IN_IDLE_STATE           0x01    /**< Card in idle state */
#define MSD_ERASE_RESET             0x02    /**< Erase reset */
#define MSD_ILLEGAL_COMMAND         0x04    /**< Illegal command */
#define MSD_COM_CRC_ERROR           0x08    /**< Command CRC error */
#define MSD_ERASE_SEQUENCE_ERROR    0x10    /**< Erase sequence error */
#define MSD_ADDRESS_ERROR           0x20    /**< Address error */
#define MSD_PARAMETER_ERROR         0x40    /**< Parameter error */
#define MSD_RESPONSE_FAILURE        0xFF    /**< Response failure */

//------------------------------------------------------------------------------------------------------------------------------------------------

BYTE SD_Type = 0; /**< SD card type */

//------------------------------------------------------------------------------------------------------------------------------------------------

/** @brief Initialize the MMC disk.
 *  @return Disk status.
 */
DSTATUS MMC_disk_initialize(void) {
    DSTATUS state;
    state = SD_Init();
    if(state == STA_NODISK) {
        return STA_NODISK;
    }
    else if(state != 0) {
        return STA_NOINIT;  // other error: initialization failed
    }
    else {
        return 0;           // initialization succeeded
    }
}

/** @brief Write data to the MMC disk.
 *  @param buff Pointer to the data buffer.
 *  @param sector Sector number.
 *  @param count Number of sectors to write.
 *  @return Operation result.
 */
DRESULT MMC_disk_write(const BYTE *buff, DWORD sector, UINT count) {
    BYTE res;
    if(count == 1) {
        res = SD_WriteSingleBlock(sector, buff);
    }
    else {
        res = SD_WriteMultiBlock(sector, buff, count);
    }
    if(res == 0) {
        return RES_OK;
    }
    else {
        return RES_ERROR;
    }
}

/** @brief Read data from the MMC disk.
 *  @param buff Pointer to the data buffer.
 *  @param sector Sector number.
 *  @param count Number of sectors to read.
 *  @return Operation result.
 */
DRESULT MMC_disk_read(BYTE *buff, DWORD sector, UINT count) {
    BYTE res = 0;
    if(count==1) {
        res = SD_ReadSingleBlock(sector, buff);
    }
    else {
        res = SD_ReadMultiBlock(sector, buff, count);
    }
    if(res == 0x00) {
        return RES_OK;
    }
    else {
        return RES_ERROR;
    }
}
/** @brief Perform disk I/O control operations.
 *  @param ctrl Control code.
 *  @param buff Pointer to the control data buffer.
 *  @return Operation result.
 */
DRESULT MMC_disk_ioctl ( BYTE ctrl, void *buff ) {
    DRESULT res;
    // FATFS only deal with the current version of CTRL_SYNC, GET_SECTOR_COUNT, GET_BLOCK_SIZ three commands
    switch(ctrl) {
    case CTRL_SYNC:
        SELECT();
        if(SD_WaitDataReady()==0) {
            res = RES_OK;
        }
        else {
            res = RES_ERROR;
        }
        DESELECT();
        break;
    case GET_BLOCK_SIZE:
        *(WORD*)buff = 512;
        res = RES_OK;
        break;
    case GET_SECTOR_COUNT:
        *(DWORD*)buff = SD_GetCapacity();
        res = RES_OK;
        break;
    default:
        res = RES_PARERR;
        break;
    }
    return res;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Initialize the SD card.
 *
 * This function initializes the SD card by sending initialization commands and
 * determining the type of SD card present. It communicates with the SD card
 * via SPI.
 *
 * @return SD card status.
 */
DSTATUS SD_Init(void) {
    BYTE r1; ///< Return value of SD card storage
    DWORD retry; ///< Used to count out
    BYTE buff[6] = {0, 0, 0, 0, 0}; ///< Buffer for storing response data
    DESELECT(); // Deselect the SD card
    bsp_readwritebyte_spi2(0xff); // Send dummy data to prepare SPI communication
    if (SD_Idle_Sta()) {
        return 1; // Return if SD card is in idle state
    }
    SELECT(); // Select the SD card
    // Send CMD8 command to initialize SD card
    r1 = SD_SendCommand_NoDeassert(8, 0x1aa, 0x87);
    // Determine the type of SD card
    if (r1 == 0x05) {
        SD_Type = SD_TYPE_V1; // Version 1.x Standard Capacity SD card
        SELECT();
        bsp_readwritebyte_spi2(0xFF); // Send dummy data
        retry = 0;
        do {
            r1 = SD_SendCommand(CMD55, 0, 0); // Send CMD55
            if (r1 == 0XFF) {
                return r1;
            }
            r1 = SD_SendCommand(ACMD41, 0, 0); // Send ACMD41
            retry++;
        } while ((r1 != 0x00) && (retry < 400));
        if (retry == 400) {
            retry = 0;
            do {
                r1 = SD_SendCommand(1, 0, 0); // Send CMD1
                retry++;
            } while ((r1 != 0x00) && (retry < 400));
            if (retry == 400) {
                return 1;
            }
            SD_Type = SD_TYPE_MMC; // MultiMediaCard (MMC)
        }
        bsp_readwritebyte_spi2(0xFF); // Send dummy data
        // Send CMD59 command to enable CRC
        r1 = SD_SendCommand(CMD59, 0, 0x95);
        if (r1 != 0x00) {
            return r1;
        }
        // Send CMD16 command to set block length
        r1 = SD_SendCommand(CMD16, 512, 0x95);
        if (r1 != 0x00) {
            return r1;
        }
    } else if (r1 == 0x01) {
        // Version 2.0 Standard Capacity SD card
        buff[0] = bsp_readwritebyte_spi2(0xFF); // Read response byte 1
        buff[1] = bsp_readwritebyte_spi2(0xFF); // Read response byte 2
        buff[2] = bsp_readwritebyte_spi2(0xFF); // Read response byte 3
        buff[3] = bsp_readwritebyte_spi2(0xFF); // Read response byte 4
        DESELECT(); // Deselect the SD card
        bsp_readwritebyte_spi2(0xFF); // Send dummy data
        retry = 0;
        do {
            // Send CMD55 command
            r1 = SD_SendCommand(CMD55, 0, 0);
            if (r1 != 0x01) return r1;

            // Send ACMD41 command
            r1 = SD_SendCommand(ACMD41, 0x40000000, 0);
            if (retry > 200) return r1; // Timeout, return status r1
        } while (r1 != 0);
        // Send CMD58 command to read OCR information
        r1 = SD_SendCommand_NoDeassert(CMD58, 0, 0);
        if (r1 != 0x00) {
            DESELECT();
            return r1;
        }
        // Read OCR information
        buff[0] = bsp_readwritebyte_spi2(0xFF); // Read response byte 1
        buff[1] = bsp_readwritebyte_spi2(0xFF); // Read response byte 2
        buff[2] = bsp_readwritebyte_spi2(0xFF); // Read response byte 3
        buff[3] = bsp_readwritebyte_spi2(0xFF); // Read response byte 4
        DESELECT(); // Deselect the SD card
        bsp_readwritebyte_spi2(0xFF); // Send dummy data
        if (buff[0] & 0x40) {
            SD_Type = SD_TYPE_V2HC; // Version 2.0 High Capacity SD card (SDHC)
        } else {
            SD_Type = SD_TYPE_V2; // Version 2.0 Standard Capacity SD card
        }
    }
    return r1; // Return SD card status
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Select the SD card.
 *
 * This function selects the SD card by setting the corresponding GPIO pin to
 * a logic low level.
 */
void SELECT(void)
{
  //GpioDataRegs.GPBCLEAR.bit.GPIO61 = 1; // Set GPIO61 to logic low
  gpioPinWrite(15, _DEF_LOW);
}

/**
 * @brief Deselect the SD card.
 *
 * This function deselects the SD card by setting the corresponding GPIO pin to
 * a logic high level.
 */
void DESELECT(void)
{
  //GpioDataRegs.GPBSET.bit.GPIO61 = 1; // Set GPIO61 to logic high
  gpioPinWrite(15, _DEF_HIGH);
}
//------------------------------------------------------------------------------------------------------------------------------------------------
/**
 * @brief Write data to a single block on the SD card.
 *
 * This function writes data to a single block on the SD card.
 *
 * @param sector The sector address to write data to.
 * @param data Pointer to the data to be written.
 * @return Status byte indicating success or failure.
 */
BYTE SD_WriteSingleBlock(DWORD sector, const BYTE *data) {
    BYTE r1; ///< Return value of SD card storage
    DWORD i; ///< Loop counter
    DWORD retry; ///< Used to count out
    // Adjust sector address for non-HC cards
    if (SD_Type != SD_TYPE_V2HC) {
        sector = sector << 9;
    }
    // Send CMD24 command to specify block address for writing
    r1 = SD_SendCommand(CMD24, sector, 0x00);
    if (r1 != 0x00) {
        return r1; // Response is not correct, return error
    }
    SELECT(); // Select the SD card
    // Send data start token
    bsp_readwritebyte_spi2(0xff);
    bsp_readwritebyte_spi2(0xff);
    bsp_readwritebyte_spi2(0xff);
    bsp_readwritebyte_spi2(0xFE);
    // Write data block
    for (i = 0; i < 512; i++) {
        bsp_readwritebyte_spi2(*data++);
    }
    // Send CRC and dummy bytes
    bsp_readwritebyte_spi2(0xff);
    bsp_readwritebyte_spi2(0xff);
    // Read data response
    r1 = bsp_readwritebyte_spi2(0xff);
    if ((r1 & 0x1F) != 0x05) {
        DESELECT(); // Deselect the SD card
        return r1; // Return error
    }
    // Wait for data to be written
    retry = 0;
    while (!bsp_readwritebyte_spi2(0xff)) {
        retry++;
        if (retry > 0xfffe) {
            DESELECT(); // Deselect the SD card
            return 1; // Return error
        }
    }
    DESELECT(); // Deselect the SD card
    bsp_readwritebyte_spi2(0xff);
    return 0; // Return success
}

/**
 * @brief Write data to multiple blocks on the SD card.
 *
 * This function writes data to multiple blocks on the SD card.
 *
 * @param sector The starting sector address to write data to.
 * @param data Pointer to the data to be written.
 * @param count Number of blocks to write.
 * @return Status byte indicating success or failure.
 */
BYTE SD_WriteMultiBlock(DWORD sector, const BYTE *data, BYTE count) {
    BYTE r1; ///< Return value of SD card storage
    DWORD i; ///< Loop counter
    // Adjust sector address for non-HC cards
    if (SD_Type != SD_TYPE_V2HC) {
        sector = sector << 9;
    }
    // Send ACMD23 command to set the number of blocks to write (for non-MMC cards)
    if (SD_Type != SD_TYPE_MMC) {
        r1 = SD_SendCommand(ACMD23, count, 0x00);
    }
    // Send CMD25 command to specify block address for writing
    r1 = SD_SendCommand(CMD25, sector, 0x00);
    if (r1 != 0x00) {
        return r1; // Return error
    }
    SELECT(); // Select the SD card
    // Write multiple data blocks
    do {
        bsp_readwritebyte_spi2(0xff); // Send data start token 0xFC
        for (i = 0; i < 512; i++) {
            bsp_readwritebyte_spi2(*data++); // Write data block
        }
        bsp_readwritebyte_spi2(0xff); // Send CRC and dummy bytes
        // Read data response
        r1 = bsp_readwritebyte_spi2(0xff);
        if ((r1 & 0x1F) != 0x05) {
            DESELECT(); // Deselect the SD card
            return r1; // Return error
        }
        // Check if data is ready
        if (SD_WaitDataReady() == 1) {
            DESELECT(); // Deselect the SD card
            return 1; // Return error
        }
    } while (--count);
    // Send stop token
    r1 = bsp_readwritebyte_spi2(0xFD);
    if (r1 == 0x00) {
        count = 0xFE;
    }
    // Check if data is ready
    if (SD_WaitDataReady()) {
        DESELECT(); // Deselect the SD card
        return 1; // Return error
    }
    DESELECT(); // Deselect the SD card
    bsp_readwritebyte_spi2(0xff);
    return count; // Return status byte
}

uint16_t read_error_index = 0;
uint16_t read_cmd_error_index = 0;
/**
 * @brief Read data from a single block on the SD card.
 *
 * This function reads data from a single block on the SD card.
 *
 * @param sector The sector address to read data from.
 * @param buffer Pointer to the buffer to store the read data.
 * @return Status byte indicating success or failure.
 */
BYTE SD_ReadSingleBlock(DWORD sector, BYTE *buffer) {
    BYTE r1; ///< Return value of SD card storage
    // Adjust sector address for non-HC cards
    if (SD_Type != SD_TYPE_V2HC) {
        sector = sector << 9;
    }
    // Send CMD17 command to specify block address for reading
    r1 = SD_SendCommand(CMD17, sector, 0); // read command
    read_cmd_error_index = r1; // Save command error index
    if (r1 != 0x00) {
        return r1; // Return error
    }
    // Receive data from SD card
    r1 = SD_ReceiveData(buffer, 512, RELEASE);
    read_error_index = r1; // Save read error index
    if (r1 != 0) {
        return r1; // Return read data error
    } else {
        return 0; // Return success
    }
}

/**
 * @brief Read data from multiple blocks on the SD card.
 *
 * This function reads data from multiple blocks on the SD card.
 *
 * @param sector The starting sector address to read data from.
 * @param buffer Pointer to the buffer to store the read data.
 * @param count Number of blocks to read.
 * @return Status byte indicating success or failure.
 */
BYTE SD_ReadMultiBlock(DWORD sector, BYTE *buffer, BYTE count)
{
    BYTE r1; ///< Return value of SD card storage
    // Adjust sector address for non-HC cards
    if (SD_Type != SD_TYPE_V2HC) {
        sector = sector << 9;
    }
    // Send CMD18 command to specify block address for reading
    r1 = SD_SendCommand(CMD18, sector, 0);
    if (r1 != 0x00) {
        return r1; // Return error
    }
    // Read multiple data blocks
    do {
        // Receive data from SD card without releasing the bus
        if (SD_ReceiveData(buffer, 512, NO_RELEASE) != 0x00) {
            break;
        }
        buffer += 512; // Move buffer pointer to next block
    } while (--count);
    // Send CMD12 command to stop data transfer
    SD_SendCommand(CMD12, 0, 0);
    DESELECT(); // Deselect the SD card
    bsp_readwritebyte_spi2(0xFF); // Clock out remaining bytes

    if (count != 0) {
        return count; // Return count if not all blocks were read
    } else {
        return 0; // Return success if all blocks were read
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------------

//SD_SendCommand
//bsp_readwritebyte_spi2
//SD_ReceiveData
//SD_WaitDataReady
/**
 * @brief Send a command to the SD card with chip select and deassert.
 *
 * This function sends a command to the SD card with chip select and deassert.
 *
 * @param cmd The command byte to send.
 * @param arg The argument for the command.
 * @param crc The CRC byte for the command.
 * @return Response byte from the SD card.
 */
BYTE SD_SendCommand(BYTE cmd, DWORD arg, BYTE crc) {
    BYTE r1; ///< Response byte from the SD card
    BYTE Retry = 0; ///< Retry counter for waiting response
    // Close the chip select
    DESELECT();
    // High-speed write command delay
    bsp_readwritebyte_spi2(0xff);
    bsp_readwritebyte_spi2(0xff);
    bsp_readwritebyte_spi2(0xff);
    // Select the SD card by bringing chip select low
    SELECT();
    // Send the command
    bsp_readwritebyte_spi2(cmd | 0x40); // Write command
    bsp_readwritebyte_spi2(arg >> 24); // Send command argument
    bsp_readwritebyte_spi2(arg >> 16);
    bsp_readwritebyte_spi2(arg >> 8);
    bsp_readwritebyte_spi2(arg);
    bsp_readwritebyte_spi2(crc); // Send CRC
    // Wait for a response or timeout
    while ((r1 = bsp_readwritebyte_spi2(0xFF)) == 0xFF) {
        Retry++;
        if (Retry > 200) {
            break;
        }
    }
    // Close the chip select
    DESELECT();
    // Additional 8 clocks on the bus to complete SD card's remaining work
    bsp_readwritebyte_spi2(0xFF);
    // Return the response byte
    return r1;
}

/**
 * @brief Send a command to the SD card without deasserting chip select.
 *
 * This function sends a command to the SD card without deasserting chip select.
 *
 * @param cmd The command byte to send.
 * @param arg The argument for the command.
 * @param crc The CRC byte for the command.
 * @return Response byte from the SD card.
 */
BYTE SD_SendCommand_NoDeassert(BYTE cmd, DWORD arg, BYTE crc) {
    BYTE Retry = 0; ///< Retry counter for waiting response
    BYTE r1; ///< Response byte from the SD card
    // High-speed write command delay
    bsp_readwritebyte_spi2(0xff);
    bsp_readwritebyte_spi2(0xff);
    // Select the SD card by bringing chip select low
    SELECT();
    // Send the command
    bsp_readwritebyte_spi2(cmd | 0x40); // Write command
    bsp_readwritebyte_spi2(arg >> 24); // Send command argument
    bsp_readwritebyte_spi2(arg >> 16);
    bsp_readwritebyte_spi2(arg >> 8);
    bsp_readwritebyte_spi2(arg);
    bsp_readwritebyte_spi2(crc); // Send CRC

    // Wait for a response or timeout
    while ((r1 = bsp_readwritebyte_spi2(0xFF)) == 0xFF) {
        Retry++;
        if (Retry > 200) {
            break;
        }
    }
    // Return the response byte
    return r1;
}

/**
 * @brief Read or write a byte over SPI.
 *
 * This function reads or writes a byte over SPI.
 *
 * @param tx_data The byte to write over SPI.
 * @return The byte read from SPI.
 */
BYTE bsp_readwritebyte_spi2(BYTE tx_data) {
    return (BYTE) SpiSdcardDriver((uint16_t) tx_data);
}

/**
 * @brief Receive data from the SD card.
 *
 * This function receives data from the SD card.
 *
 * @param data Pointer to the buffer where the received data will be stored.
 * @param len Length of the data to receive.
 * @param release Flag indicating whether to release the chip select after receiving data.
 * @return 0 on success, 1 if there was an error receiving data.
 */
BYTE SD_ReceiveData(BYTE *data, DWORD len, BYTE release) {
    SELECT();
    if (SD_GetResponse(0xFE)) {
        DESELECT();
        return 1;
    }

    while (len--) {
        *data = bsp_readwritebyte_spi2(0xFF);
        data++;
    }

    bsp_readwritebyte_spi2(0xFF);
    bsp_readwritebyte_spi2(0xFF);

    if (release == RELEASE) {
        DESELECT();
        bsp_readwritebyte_spi2(0xFF);
    }
    return 0;
}

/**
 * @brief Wait for data ready signal from the SD card.
 *
 * This function waits for the data ready signal from the SD card.
 *
 * @return Status byte indicating the result of the operation.
 */
BYTE SD_WaitDataReady(void) {
    BYTE r1 = MSD_DATA_OTHER_ERROR;
    DWORD retry = 0;
    do {
        r1 = bsp_readwritebyte_spi2(0xFF) & 0x1F; // Read response
        if (retry == 0xfffe) {
            return 1;
        }
        retry++;

        switch (r1) {
            case MSD_DATA_OK:
                r1 = MSD_DATA_OK;
                break;
            case MSD_DATA_CRC_ERROR:
                return MSD_DATA_CRC_ERROR;
            case MSD_DATA_WRITE_ERROR:
                return MSD_DATA_WRITE_ERROR;
            default:
                r1 = MSD_DATA_OTHER_ERROR;
                break;
        }
    } while (r1 == MSD_DATA_OTHER_ERROR); // Wait for data errors

    retry = 0;
    while (bsp_readwritebyte_spi2(0xFF) == 0) {
        retry++;
        if (retry >= 0XFFFFFFFE) {
            return 0XFF; // Wait failed
        }
    };
    return 0; // Success
}

/**
 * @brief Get response from the SD card.
 *
 * This function gets a response from the SD card.
 *
 * @param Response The expected response byte.
 * @return Status byte indicating the result of the operation.
 */
BYTE SD_GetResponse(BYTE Response) {
    DWORD Count = 0xFFF;

    while ((bsp_readwritebyte_spi2(0XFF) != Response) && Count) {
        Count--;
    }

    if (Count == 0) {
        return MSD_RESPONSE_FAILURE; // Response failure
    } else {
        return MSD_RESPONSE_NO_ERROR; // Correct response
    }
}

/**
 * @brief Check the idle state of the SD card.
 *
 * This function checks the idle state of the SD card.
 *
 * @return 0 if the SD card is idle, 1 if there was an error.
 */
BYTE SD_Idle_Sta(void) {
    DWORD i;
    BYTE r1;
    BYTE retry;
    for (i = 0; i < 0xf00; i++); // Pure delay, waiting for SD card to complete power-up
    // Should generate > 74 pulses to allow SD card to complete initialization
    for (i = 0; i < 10; i++) {
        bsp_readwritebyte_spi2(0xFF);
    }
    retry = 0;
    do {
        r1 = SD_SendCommand(CMD0, 0, 0x95);
        retry++;
    } while ((r1 != 0x01) && (retry < 200));

    if (retry == 200) {
        return 1;
    }
    return 0;
}
/**
 * @brief Get the capacity of the SD card in bytes.
 *
 * This function retrieves the capacity of the SD card in bytes.
 *
 * @return Capacity of the SD card in bytes. Returns 0 if there is an error.
 */
DWORD SD_GetCapacity(void) {
    BYTE csd[16];
    DWORD Capacity;
    BYTE r1;
    BYTE i;
    BYTE temp;
    // Get CSD information, return 0 if there's an error
    if (SD_GetCSD(csd) != 0) {
        return 0;
    }
    // If it's an SDHC card, calculate capacity accordingly
    if ((csd[0] & 0xC0) == 0x40) {
        Capacity = ((DWORD)csd[8]) << 8;
        Capacity += (DWORD)csd[9] + 1;
        Capacity = (Capacity) * 1024; // Get number of sectors
        Capacity *= 512; // Get number of bytes
    } else {
        i = csd[6] & 0x03;
        i <<= 8;
        i += csd[7];
        i <<= 2;
        i += ((csd[8] & 0xc0) >> 6);
        // C_SIZE_MULT
        r1 = csd[9] & 0x03;
        r1 <<= 1;
        r1 += ((csd[10] & 0x80) >> 7);
        r1 += 2; // BLOCKNR
        temp = 1;
        while (r1) {
            temp *= 2;
            r1--;
        }
        Capacity = ((DWORD)(i + 1)) * ((DWORD)temp);
        // READ_BL_LEN
        i = csd[5] & 0x0f;
        // BLOCK_LEN
        temp = 1;
        while (i) {
            temp *= 2;
            i--;
        }
        // The final result
        Capacity *= (DWORD)temp; // in bytes
    }
    return (DWORD)Capacity;
}

/**
 * @brief Get the Card Specific Data (CSD) register from the SD card.
 *
 * This function retrieves the Card Specific Data (CSD) register from the SD card.
 *
 * @param csd_data Pointer to the buffer where the CSD register will be stored.
 * @return 0 on success, error code otherwise.
 */
BYTE SD_GetCSD(BYTE *csd_data) {
    BYTE r1;
    r1 = SD_SendCommand(CMD9, 0, 0xff); // Send CMD9 command, read CSD
    if (r1) {
        return r1; // If the correct response is not received, return an error
    }
    SD_ReceiveData(csd_data, 16, RELEASE); // Receive 16 bytes of data
    return 0;
}

//------------------------------------------------------------------------------------------------------------------------------------------------
