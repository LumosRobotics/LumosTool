#include "spi_config.h"
#include "sdcard.h"
#include <string.h>

/**
 * @brief Print card type (for debugging with UART)
 */
const char* SDCard_TypeToString(SDCard_Type_t type)
{
    switch (type) {
        case SDCARD_TYPE_V1:   return "SD Ver 1.x";
        case SDCARD_TYPE_V2:   return "SD Ver 2.0 (SDSC)";
        case SDCARD_TYPE_SDHC: return "SD Ver 2.0 (SDHC)";
        default:               return "Unknown";
    }
}

int main(void)
{
    SDCard_Error_t sd_error;
    SDCard_Type_t sd_type;
    uint8_t read_buffer[512];
    uint8_t write_buffer[512];

    // Initialize HAL Library
    HAL_Init();

    // Configure system clock to 550 MHz
    SystemClock_Config();

    // Initialize SPI
    SPI_Init();

    // Give SD card time to power up
    HAL_Delay(100);

    // Initialize SD card
    sd_error = SDCard_Init();

    if (sd_error == SDCARD_OK) {
        // SD card initialized successfully
        sd_type = SDCard_GetType();

        // In a real application, you would output this information via UART
        // For now, we just store the type
        (void)sd_type;  // Suppress unused variable warning

        // Example 1: Read block 0 (Master Boot Record / Partition Table)
        // This contains filesystem information
        sd_error = SDCard_ReadBlock(0, read_buffer);

        if (sd_error == SDCARD_OK) {
            // Successfully read block 0
            // Bytes 510-511 should contain 0x55AA (boot signature)
            uint8_t boot_signature_valid = (read_buffer[510] == 0x55 && read_buffer[511] == 0xAA);
            (void)boot_signature_valid;  // In real app, output via UART

            // The buffer now contains the first sector of the SD card
            // You can examine partition table at offset 0x1BE
        }

        // Example 2: Write and read back data
        // Prepare test data
        for (uint16_t i = 0; i < 512; i++) {
            write_buffer[i] = (uint8_t)(i & 0xFF);
        }

        // Write to block 1000 (away from filesystem structures)
        // WARNING: This will overwrite any data at block 1000!
        // Comment out if you don't want to modify SD card contents
        /*
        sd_error = SDCard_WriteBlock(1000, write_buffer);

        if (sd_error == SDCARD_OK) {
            // Read back and verify
            sd_error = SDCard_ReadBlock(1000, read_buffer);

            if (sd_error == SDCARD_OK) {
                // Compare buffers
                uint8_t data_matches = (memcmp(read_buffer, write_buffer, 512) == 0);
                (void)data_matches;  // In real app, output via UART
            }
        }
        */

        // Example 3: Read multiple blocks
        // If you have a FAT filesystem, you can read and parse it
        // Block 0: MBR/Partition Table
        // Blocks 1-N: FAT tables and data

    } else {
        // SD card initialization failed
        // In real application, you would signal error via LED or UART
    }

    // Main loop
    while (1)
    {
        // Periodically read from SD card
        sd_error = SDCard_ReadBlock(0, read_buffer);

        if (sd_error == SDCARD_OK) {
            // Successfully read data
            // In a real application, you might:
            // 1. Read configuration data
            // 2. Read logged data
            // 3. Update display with SD card contents
            // 4. Transfer data to another device
        } else {
            // Read failed - handle error
            // Maybe retry or signal error condition
        }

        // Wait before next operation
        HAL_Delay(1000);
    }

    return 0;
}

// SysTick interrupt handler (required by HAL_Delay)
void SysTick_Handler(void)
{
    HAL_IncTick();
}
