#include "sdcard.h"
#include "spi_config.h"

// SD Card state
static SDCard_Type_t card_type = SDCARD_TYPE_UNKNOWN;
static uint32_t card_capacity = 0;  // in blocks

/**
 * @brief Send SD card command
 * @param cmd Command index
 * @param arg Command argument
 * @return R1 response
 */
static uint8_t SDCard_SendCommand(uint8_t cmd, uint32_t arg)
{
    uint8_t response;
    uint8_t crc = 0xFF;
    uint16_t timeout;

    // Special CRC for CMD0 and CMD8
    if (cmd == CMD0) crc = 0x95;
    if (cmd == CMD8) crc = 0x87;

    // Send command packet
    SPI_TransferByte(0x40 | cmd);           // Command with start bit
    SPI_TransferByte((arg >> 24) & 0xFF);   // Argument[31:24]
    SPI_TransferByte((arg >> 16) & 0xFF);   // Argument[23:16]
    SPI_TransferByte((arg >> 8) & 0xFF);    // Argument[15:8]
    SPI_TransferByte(arg & 0xFF);           // Argument[7:0]
    SPI_TransferByte(crc);                  // CRC + stop bit

    // Wait for response (R1 format: 0xxxxxxx)
    timeout = 10;
    do {
        response = SPI_TransferByte(0xFF);
        timeout--;
    } while ((response & 0x80) && timeout);

    return response;
}

/**
 * @brief Send application-specific command (preceded by CMD55)
 * @param cmd ACMD command index
 * @param arg Command argument
 * @return R1 response
 */
static uint8_t SDCard_SendAppCommand(uint8_t cmd, uint32_t arg)
{
    SDCard_SendCommand(CMD55, 0);
    return SDCard_SendCommand(cmd, arg);
}

/**
 * @brief Wait for SD card to be ready
 * @return SDCARD_OK if ready, error otherwise
 */
static SDCard_Error_t SDCard_WaitReady(void)
{
    uint16_t timeout = 500;  // 500ms timeout
    uint8_t response;

    do {
        response = SPI_TransferByte(0xFF);
        if (response == 0xFF) return SDCARD_OK;
        HAL_Delay(1);
        timeout--;
    } while (timeout);

    return SDCARD_ERROR_TIMEOUT;
}

/**
 * @brief Initialize SD card
 * @return SDCARD_OK if successful, error code otherwise
 */
SDCard_Error_t SDCard_Init(void)
{
    uint8_t response;
    uint16_t timeout;
    uint8_t ocr[4];

    card_type = SDCARD_TYPE_UNKNOWN;

    // Power up sequence: send at least 74 clock pulses with CS high
    SPI_CS_HIGH();
    for (int i = 0; i < 10; i++) {
        SPI_TransferByte(0xFF);
    }
    HAL_Delay(10);

    // Enter SPI mode: Send CMD0
    SPI_CS_LOW();
    response = SDCard_SendCommand(CMD0, 0);
    SPI_CS_HIGH();

    if (response != 0x01) {
        return SDCARD_ERROR_INIT;  // Card not in idle state
    }

    // Check card version: Send CMD8
    SPI_CS_LOW();
    response = SDCard_SendCommand(CMD8, 0x1AA);  // 2.7-3.6V, check pattern 0xAA

    if (response == 0x01) {
        // SD Ver 2.0 or later
        // Read R7 response (4 bytes)
        for (int i = 0; i < 4; i++) {
            ocr[i] = SPI_TransferByte(0xFF);
        }
        SPI_CS_HIGH();

        // Check if card supports 2.7-3.6V and echo back matches
        if ((ocr[2] != 0x01) || (ocr[3] != 0xAA)) {
            return SDCARD_ERROR_INIT;
        }

        // Initialize card with ACMD41
        timeout = 1000;  // 1 second timeout
        do {
            SPI_CS_LOW();
            response = SDCard_SendAppCommand(ACMD41, 0x40000000);  // HCS bit set
            SPI_CS_HIGH();
            if (response == 0x00) break;
            HAL_Delay(1);
            timeout--;
        } while (timeout);

        if (timeout == 0) {
            return SDCARD_ERROR_TIMEOUT;
        }

        // Read OCR to check if it's SDHC
        SPI_CS_LOW();
        response = SDCard_SendCommand(CMD58, 0);
        if (response == 0x00) {
            for (int i = 0; i < 4; i++) {
                ocr[i] = SPI_TransferByte(0xFF);
            }
            // Check CCS bit (Card Capacity Status)
            if (ocr[0] & 0x40) {
                card_type = SDCARD_TYPE_SDHC;  // High Capacity
            } else {
                card_type = SDCARD_TYPE_V2;    // Standard Capacity
            }
        }
        SPI_CS_HIGH();

    } else {
        // SD Ver 1.x or MMC
        SPI_CS_HIGH();

        // Try to initialize with ACMD41
        timeout = 1000;
        do {
            SPI_CS_LOW();
            response = SDCard_SendAppCommand(ACMD41, 0);
            SPI_CS_HIGH();
            if (response == 0x00) break;
            HAL_Delay(1);
            timeout--;
        } while (timeout);

        if (timeout == 0) {
            return SDCARD_ERROR_TIMEOUT;
        }

        card_type = SDCARD_TYPE_V1;

        // Set block size to 512 bytes for SDSC
        SPI_CS_LOW();
        SDCard_SendCommand(CMD16, 512);
        SPI_CS_HIGH();
    }

    // Increase SPI speed after initialization (can go up to 25 MHz for SD cards)
    // Set to ~8.6 MHz (137.5 MHz / 16)
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    HAL_SPI_Init(&hspi1);

    return SDCARD_OK;
}

/**
 * @brief Get SD card type
 * @return Card type
 */
SDCard_Type_t SDCard_GetType(void)
{
    return card_type;
}

/**
 * @brief Get SD card capacity in blocks
 * @return Capacity in 512-byte blocks
 */
uint32_t SDCard_GetCapacity(void)
{
    return card_capacity;
}

/**
 * @brief Read a single block from SD card
 * @param block_addr Block address (for SDHC) or byte address (for SDSC)
 * @param buffer Pointer to 512-byte buffer
 * @return SDCARD_OK if successful, error code otherwise
 */
SDCard_Error_t SDCard_ReadBlock(uint32_t block_addr, uint8_t* buffer)
{
    uint8_t response;
    uint16_t timeout;

    // For SDSC cards, convert block address to byte address
    if (card_type != SDCARD_TYPE_SDHC) {
        block_addr *= 512;
    }

    // Send READ_SINGLE_BLOCK command
    SPI_CS_LOW();
    response = SDCard_SendCommand(CMD17, block_addr);

    if (response != 0x00) {
        SPI_CS_HIGH();
        return SDCARD_ERROR_READ;
    }

    // Wait for data token (0xFE)
    timeout = 1000;
    do {
        response = SPI_TransferByte(0xFF);
        if (response == 0xFE) break;
        HAL_Delay(1);
        timeout--;
    } while (timeout);

    if (timeout == 0) {
        SPI_CS_HIGH();
        return SDCARD_ERROR_TIMEOUT;
    }

    // Read 512 bytes of data
    for (uint16_t i = 0; i < 512; i++) {
        buffer[i] = SPI_TransferByte(0xFF);
    }

    // Read 2-byte CRC (and discard)
    SPI_TransferByte(0xFF);
    SPI_TransferByte(0xFF);

    SPI_CS_HIGH();

    // Send 8 dummy clocks
    SPI_TransferByte(0xFF);

    return SDCARD_OK;
}

/**
 * @brief Write a single block to SD card
 * @param block_addr Block address (for SDHC) or byte address (for SDSC)
 * @param buffer Pointer to 512-byte buffer
 * @return SDCARD_OK if successful, error code otherwise
 */
SDCard_Error_t SDCard_WriteBlock(uint32_t block_addr, const uint8_t* buffer)
{
    uint8_t response;

    // For SDSC cards, convert block address to byte address
    if (card_type != SDCARD_TYPE_SDHC) {
        block_addr *= 512;
    }

    // Send WRITE_BLOCK command
    SPI_CS_LOW();
    response = SDCard_SendCommand(CMD24, block_addr);

    if (response != 0x00) {
        SPI_CS_HIGH();
        return SDCARD_ERROR_WRITE;
    }

    // Send data token (0xFE)
    SPI_TransferByte(0xFE);

    // Write 512 bytes of data
    for (uint16_t i = 0; i < 512; i++) {
        SPI_TransferByte(buffer[i]);
    }

    // Send dummy CRC (2 bytes)
    SPI_TransferByte(0xFF);
    SPI_TransferByte(0xFF);

    // Read data response
    response = SPI_TransferByte(0xFF);
    if ((response & 0x1F) != 0x05) {
        // Data not accepted
        SPI_CS_HIGH();
        return SDCARD_ERROR_WRITE;
    }

    // Wait for write to complete (card will be busy)
    if (SDCard_WaitReady() != SDCARD_OK) {
        SPI_CS_HIGH();
        return SDCARD_ERROR_TIMEOUT;
    }

    SPI_CS_HIGH();

    // Send 8 dummy clocks
    SPI_TransferByte(0xFF);

    return SDCARD_OK;
}
