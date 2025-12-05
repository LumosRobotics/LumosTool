#include "sd.h"

SDCard::SDCard(SDMMC_TypeDef* sdmmc_instance) : sd_handle_{}, initialized_(false)
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
    HAL_SD_DeInit(&sd_handle_);
    initialized_ = false;
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
