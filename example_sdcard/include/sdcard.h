#pragma once

#include <stdint.h>
#include "stm32h7xx_hal.h"

// SD Card Types
typedef enum {
    SDCARD_TYPE_UNKNOWN = 0,
    SDCARD_TYPE_V1,       // SD Ver 1.x
    SDCARD_TYPE_V2,       // SD Ver 2.0 (Standard Capacity)
    SDCARD_TYPE_SDHC      // SD Ver 2.0 (High Capacity)
} SDCard_Type_t;

// SD Card Error Codes
typedef enum {
    SDCARD_OK = 0,
    SDCARD_ERROR_INIT,
    SDCARD_ERROR_TIMEOUT,
    SDCARD_ERROR_READ,
    SDCARD_ERROR_WRITE,
    SDCARD_ERROR_CRC
} SDCard_Error_t;

// SD Card Commands
#define CMD0    0   // GO_IDLE_STATE
#define CMD8    8   // SEND_IF_COND
#define CMD9    9   // SEND_CSD
#define CMD10   10  // SEND_CID
#define CMD12   12  // STOP_TRANSMISSION
#define CMD16   16  // SET_BLOCKLEN
#define CMD17   17  // READ_SINGLE_BLOCK
#define CMD18   18  // READ_MULTIPLE_BLOCK
#define CMD23   23  // SET_BLOCK_COUNT
#define CMD24   24  // WRITE_BLOCK
#define CMD25   25  // WRITE_MULTIPLE_BLOCK
#define CMD55   55  // APP_CMD
#define CMD58   58  // READ_OCR
#define ACMD41  41  // SD_SEND_OP_COND (preceded by CMD55)

// SD Card block size
#define SDCARD_BLOCK_SIZE   512

// Function prototypes
SDCard_Error_t SDCard_Init(void);
SDCard_Type_t SDCard_GetType(void);
uint32_t SDCard_GetCapacity(void);
SDCard_Error_t SDCard_ReadBlock(uint32_t block_addr, uint8_t* buffer);
SDCard_Error_t SDCard_WriteBlock(uint32_t block_addr, const uint8_t* buffer);
