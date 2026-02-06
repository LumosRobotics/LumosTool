#pragma once

// Platform-specific HAL headers
#if defined(STM32H7)
    #include "stm32h7xx_hal.h"
    #include "stm32h7xx_hal_sd.h"
#elif defined(STM32G0)
    #include "stm32g0xx_hal.h"
    // Note: Not all G0 devices have SDMMC peripheral
    #ifdef HAL_SD_MODULE_ENABLED
        #include "stm32g0xx_hal_sd.h"
    #endif
#elif defined(STM32G4)
    #include "stm32g4xx_hal.h"
    #ifdef HAL_SD_MODULE_ENABLED
        #include "stm32g4xx_hal_sd.h"
    #endif
#elif defined(STM32F4)
    #include "stm32f4xx_hal.h"
    #include "stm32f4xx_hal_sd.h"
#elif defined(STM32H5)
    #include "stm32h5xx_hal.h"
    #include "stm32h5xx_hal_sd.h"
#else
    #error "Unsupported STM32 platform. Define STM32H7, STM32G0, STM32G4, STM32F4, or STM32H5."
#endif

// SDCard Class - SD Card interface via SDMMC
// Usage Example:
//   sdcard.begin();  // Initialize SD card
//
//   // Read a block
//   uint8_t buffer[512];
//   sdcard.readBlock(0, buffer);
//
//   // Write a block
//   uint8_t data[512] = {...};
//   sdcard.writeBlock(0, data);
//
//   // Multi-block operations
//   uint8_t largeBuffer[4096];
//   sdcard.readBlocks(0, largeBuffer, 8);  // Read 8 blocks (4KB)
//
//   // Get card info
//   uint64_t capacity = sdcard.getCapacity();
//   uint32_t blockSize = sdcard.getBlockSize();

// SD card class is only available on platforms with SDMMC peripheral
#ifdef HAL_SD_MODULE_ENABLED

class SDCard
{
public:
    enum class CardType {
        UNKNOWN,
        SDSC,      // SD Standard Capacity (up to 2GB)
        SDHC,      // SD High Capacity (2GB to 32GB)
        SDXC       // SD Extended Capacity (32GB to 2TB)
    };

    enum class BusWidth {
        BUS_1BIT = 0,
        BUS_4BIT = 1
    };

private:
    SD_HandleTypeDef sd_handle_;
    bool initialized_;

    // GPIO configuration
    GPIO_TypeDef* cmd_port_;
    uint16_t cmd_pin_;
    GPIO_TypeDef* clk_port_;
    uint16_t clk_pin_;
    GPIO_TypeDef* d0_port_;
    uint16_t d0_pin_;
    GPIO_TypeDef* d1_port_;
    uint16_t d1_pin_;
    GPIO_TypeDef* d2_port_;
    uint16_t d2_pin_;
    GPIO_TypeDef* d3_port_;
    uint16_t d3_pin_;
    uint32_t alternate_function_;

public:
    SDCard() = delete;
    SDCard(SDMMC_TypeDef* sdmmc_instance,
           GPIO_TypeDef* cmd_port, uint16_t cmd_pin,
           GPIO_TypeDef* clk_port, uint16_t clk_pin,
           GPIO_TypeDef* d0_port, uint16_t d0_pin,
           GPIO_TypeDef* d1_port, uint16_t d1_pin,
           GPIO_TypeDef* d2_port, uint16_t d2_pin,
           GPIO_TypeDef* d3_port, uint16_t d3_pin,
           uint32_t alternate_function);

    // Initialization
    bool begin(BusWidth width = BusWidth::BUS_4BIT);
    void end();

    // Single block operations (512 bytes per block)
    bool readBlock(uint32_t block_address, uint8_t* buffer, uint32_t timeout = 1000);
    bool writeBlock(uint32_t block_address, const uint8_t* buffer, uint32_t timeout = 1000);

    // Multi-block operations
    bool readBlocks(uint32_t block_address, uint8_t* buffer, uint32_t num_blocks, uint32_t timeout = 5000);
    bool writeBlocks(uint32_t block_address, const uint8_t* buffer, uint32_t num_blocks, uint32_t timeout = 5000);

    // Erase operations
    bool eraseBlocks(uint32_t start_block, uint32_t end_block, uint32_t timeout = 10000);

    // Card information
    uint64_t getCapacity();          // Total capacity in bytes
    uint32_t getBlockSize();         // Block size in bytes (typically 512)
    uint32_t getBlockCount();        // Total number of blocks
    CardType getCardType();          // Card type (SDSC, SDHC, SDXC)
    bool isCardPresent();            // Check if card is inserted
    bool isWriteProtected();         // Check write protection status

    // Status
    bool isReady();
    uint32_t getError();
    HAL_SD_StateTypeDef getState();

    // Advanced configuration
    SDCard& setBusWidth(BusWidth width);
    SDCard& setClockSpeed(uint32_t clock_div);

private:
    bool waitReady(uint32_t timeout = 1000);
};

#endif // HAL_SD_MODULE_ENABLED
