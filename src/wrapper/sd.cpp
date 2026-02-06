#include "sd.h"

// SD card functionality is only available on platforms with SDMMC peripheral
#ifdef HAL_SD_MODULE_ENABLED

// Helper function to enable GPIO port clock
static void enableGPIOClock(GPIO_TypeDef* port)
{
    if (port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
#ifdef GPIOG
    else if (port == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();
#endif
#ifdef GPIOH
    else if (port == GPIOH) __HAL_RCC_GPIOH_CLK_ENABLE();
#endif
}

SDCard::SDCard(SDMMC_TypeDef* sdmmc_instance,
               GPIO_TypeDef* cmd_port, uint16_t cmd_pin,
               GPIO_TypeDef* clk_port, uint16_t clk_pin,
               GPIO_TypeDef* d0_port, uint16_t d0_pin,
               GPIO_TypeDef* d1_port, uint16_t d1_pin,
               GPIO_TypeDef* d2_port, uint16_t d2_pin,
               GPIO_TypeDef* d3_port, uint16_t d3_pin,
               uint32_t alternate_function)
    : sd_handle_{},
      initialized_(false),
      cmd_port_(cmd_port),
      cmd_pin_(cmd_pin),
      clk_port_(clk_port),
      clk_pin_(clk_pin),
      d0_port_(d0_port),
      d0_pin_(d0_pin),
      d1_port_(d1_port),
      d1_pin_(d1_pin),
      d2_port_(d2_port),
      d2_pin_(d2_pin),
      d3_port_(d3_port),
      d3_pin_(d3_pin),
      alternate_function_(alternate_function)
{
    // Initialize SD handle with default values
    sd_handle_.Instance = sdmmc_instance;

    // Default SDMMC configuration for STM32H7
    sd_handle_.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
    sd_handle_.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    sd_handle_.Init.BusWide = SDMMC_BUS_WIDE_4B;  // 4-bit mode
    sd_handle_.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
    sd_handle_.Init.ClockDiv = 2;  // Divider for clock speed
}

bool SDCard::begin(BusWidth width)
{
    // Set bus width
    if (width == BusWidth::BUS_1BIT) {
        sd_handle_.Init.BusWide = SDMMC_BUS_WIDE_1B;
    } else {
        sd_handle_.Init.BusWide = SDMMC_BUS_WIDE_4B;
    }

    // Enable GPIO port clocks
    enableGPIOClock(cmd_port_);
    enableGPIOClock(clk_port_);
    enableGPIOClock(d0_port_);
    enableGPIOClock(d1_port_);
    enableGPIOClock(d2_port_);
    enableGPIOClock(d3_port_);

    // Configure GPIO pins for SDMMC
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = alternate_function_;

    // Configure CMD pin
    GPIO_InitStruct.Pin = cmd_pin_;
    HAL_GPIO_Init(cmd_port_, &GPIO_InitStruct);

    // Configure CLK pin
    GPIO_InitStruct.Pin = clk_pin_;
    HAL_GPIO_Init(clk_port_, &GPIO_InitStruct);

    // Configure D0 pin
    GPIO_InitStruct.Pin = d0_pin_;
    HAL_GPIO_Init(d0_port_, &GPIO_InitStruct);

    // Configure D1 pin
    GPIO_InitStruct.Pin = d1_pin_;
    HAL_GPIO_Init(d1_port_, &GPIO_InitStruct);

    // Configure D2 pin
    GPIO_InitStruct.Pin = d2_pin_;
    HAL_GPIO_Init(d2_port_, &GPIO_InitStruct);

    // Configure D3 pin
    GPIO_InitStruct.Pin = d3_pin_;
    HAL_GPIO_Init(d3_port_, &GPIO_InitStruct);

    // Enable SDMMC peripheral clock
    if (sd_handle_.Instance == SDMMC1) {
        __HAL_RCC_SDMMC1_CLK_ENABLE();
    } else if (sd_handle_.Instance == SDMMC2) {
        __HAL_RCC_SDMMC2_CLK_ENABLE();
    }

    // Initialize SDMMC peripheral
    if (HAL_SD_Init(&sd_handle_) != HAL_OK) {
        initialized_ = false;
        return false;
    }

    // Configure bus width if different from initialization
    if (width == BusWidth::BUS_4BIT && sd_handle_.Init.BusWide == SDMMC_BUS_WIDE_1B) {
        if (HAL_SD_ConfigWideBusOperation(&sd_handle_, SDMMC_BUS_WIDE_4B) != HAL_OK) {
            initialized_ = false;
            return false;
        }
    }

    initialized_ = true;
    return true;
}

void SDCard::end()
{
    if (initialized_) {
        HAL_SD_DeInit(&sd_handle_);

        // Deinitialize GPIO pins
        HAL_GPIO_DeInit(cmd_port_, cmd_pin_);
        HAL_GPIO_DeInit(clk_port_, clk_pin_);
        HAL_GPIO_DeInit(d0_port_, d0_pin_);
        HAL_GPIO_DeInit(d1_port_, d1_pin_);
        HAL_GPIO_DeInit(d2_port_, d2_pin_);
        HAL_GPIO_DeInit(d3_port_, d3_pin_);

        initialized_ = false;
    }
}

bool SDCard::readBlock(uint32_t block_address, uint8_t* buffer, uint32_t timeout)
{
    if (!initialized_ || !buffer) {
        return false;
    }

    HAL_StatusTypeDef status = HAL_SD_ReadBlocks(&sd_handle_, buffer, block_address, 1, timeout);

    if (status != HAL_OK) {
        return false;
    }

    // Wait until card is ready
    return waitReady(timeout);
}

bool SDCard::writeBlock(uint32_t block_address, const uint8_t* buffer, uint32_t timeout)
{
    if (!initialized_ || !buffer) {
        return false;
    }

    HAL_StatusTypeDef status = HAL_SD_WriteBlocks(&sd_handle_, (uint8_t*)buffer, block_address, 1, timeout);

    if (status != HAL_OK) {
        return false;
    }

    // Wait until card is ready
    return waitReady(timeout);
}

bool SDCard::readBlocks(uint32_t block_address, uint8_t* buffer, uint32_t num_blocks, uint32_t timeout)
{
    if (!initialized_ || !buffer || num_blocks == 0) {
        return false;
    }

    HAL_StatusTypeDef status = HAL_SD_ReadBlocks(&sd_handle_, buffer, block_address, num_blocks, timeout);

    if (status != HAL_OK) {
        return false;
    }

    // Wait until card is ready
    return waitReady(timeout);
}

bool SDCard::writeBlocks(uint32_t block_address, const uint8_t* buffer, uint32_t num_blocks, uint32_t timeout)
{
    if (!initialized_ || !buffer || num_blocks == 0) {
        return false;
    }

    HAL_StatusTypeDef status = HAL_SD_WriteBlocks(&sd_handle_, (uint8_t*)buffer, block_address, num_blocks, timeout);

    if (status != HAL_OK) {
        return false;
    }

    // Wait until card is ready
    return waitReady(timeout);
}

bool SDCard::eraseBlocks(uint32_t start_block, uint32_t end_block, uint32_t timeout)
{
    if (!initialized_ || start_block > end_block) {
        return false;
    }

    HAL_StatusTypeDef status = HAL_SD_Erase(&sd_handle_, start_block, end_block);

    if (status != HAL_OK) {
        return false;
    }

    // Wait until erase is complete
    return waitReady(timeout);
}

uint64_t SDCard::getCapacity()
{
    if (!initialized_) {
        return 0;
    }

    HAL_SD_CardInfoTypeDef card_info;
    if (HAL_SD_GetCardInfo(&sd_handle_, &card_info) != HAL_OK) {
        return 0;
    }

    // Calculate capacity: block_count * block_size
    return (uint64_t)card_info.BlockNbr * (uint64_t)card_info.BlockSize;
}

uint32_t SDCard::getBlockSize()
{
    if (!initialized_) {
        return 0;
    }

    HAL_SD_CardInfoTypeDef card_info;
    if (HAL_SD_GetCardInfo(&sd_handle_, &card_info) != HAL_OK) {
        return 0;
    }

    return card_info.BlockSize;
}

uint32_t SDCard::getBlockCount()
{
    if (!initialized_) {
        return 0;
    }

    HAL_SD_CardInfoTypeDef card_info;
    if (HAL_SD_GetCardInfo(&sd_handle_, &card_info) != HAL_OK) {
        return 0;
    }

    return card_info.BlockNbr;
}

SDCard::CardType SDCard::getCardType()
{
    if (!initialized_) {
        return CardType::UNKNOWN;
    }

    HAL_SD_CardInfoTypeDef card_info;
    if (HAL_SD_GetCardInfo(&sd_handle_, &card_info) != HAL_OK) {
        return CardType::UNKNOWN;
    }

    switch (card_info.CardType) {
        case CARD_SDSC:
            return CardType::SDSC;
        case CARD_SDHC_SDXC:
            // Distinguish between SDHC and SDXC based on capacity
            if (getCapacity() > 32ULL * 1024ULL * 1024ULL * 1024ULL) {  // > 32GB
                return CardType::SDXC;
            } else {
                return CardType::SDHC;
            }
        default:
            return CardType::UNKNOWN;
    }
}

bool SDCard::isCardPresent()
{
    if (!initialized_) {
        return false;
    }

    // Check if card is detected (implementation depends on hardware)
    // For now, just check if we can get card info
    HAL_SD_CardInfoTypeDef card_info;
    return (HAL_SD_GetCardInfo(&sd_handle_, &card_info) == HAL_OK);
}

bool SDCard::isWriteProtected()
{
    if (!initialized_) {
        return true;  // Assume protected if not initialized
    }

    // Check write protection status (hardware dependent)
    // Most boards don't have WP pin connected, so return false
    return false;
}

bool SDCard::isReady()
{
    if (!initialized_) {
        return false;
    }

    return (HAL_SD_GetCardState(&sd_handle_) == HAL_SD_CARD_TRANSFER);
}

uint32_t SDCard::getError()
{
    return HAL_SD_GetError(&sd_handle_);
}

HAL_SD_StateTypeDef SDCard::getState()
{
    return HAL_SD_GetState(&sd_handle_);
}

SDCard& SDCard::setBusWidth(BusWidth width)
{
    if (initialized_) {
        uint32_t hal_width = (width == BusWidth::BUS_4BIT) ? SDMMC_BUS_WIDE_4B : SDMMC_BUS_WIDE_1B;
        HAL_SD_ConfigWideBusOperation(&sd_handle_, hal_width);
    }
    return *this;
}

SDCard& SDCard::setClockSpeed(uint32_t clock_div)
{
    if (initialized_) {
        sd_handle_.Init.ClockDiv = clock_div;
        HAL_SD_Init(&sd_handle_);
    }
    return *this;
}

bool SDCard::waitReady(uint32_t timeout)
{
    uint32_t start = HAL_GetTick();

    while ((HAL_GetTick() - start) < timeout) {
        if (HAL_SD_GetCardState(&sd_handle_) == HAL_SD_CARD_TRANSFER) {
            return true;
        }
        HAL_Delay(1);
    }

    return false;
}

#endif // HAL_SD_MODULE_ENABLED
