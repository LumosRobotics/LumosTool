#include "spi.h"

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

SPI::SPI(SPI_TypeDef* spi_instance,
         GPIO_TypeDef* mosi_port, uint16_t mosi_pin,
         GPIO_TypeDef* miso_port, uint16_t miso_pin,
         GPIO_TypeDef* sck_port, uint16_t sck_pin,
         uint32_t alternate_function)
    : spi_handle_{},
      mosi_port_(mosi_port),
      mosi_pin_(mosi_pin),
      miso_port_(miso_port),
      miso_pin_(miso_pin),
      sck_port_(sck_port),
      sck_pin_(sck_pin),
      alternate_function_(alternate_function)
{
    // Initialize SPI handle with default values
    spi_handle_.Instance = spi_instance;

    // Default SPI configuration (common to all platforms)
    spi_handle_.Init.Mode = SPI_MODE_MASTER;
    spi_handle_.Init.Direction = SPI_DIRECTION_2LINES;
    spi_handle_.Init.DataSize = SPI_DATASIZE_8BIT;
    spi_handle_.Init.CLKPolarity = SPI_POLARITY_LOW;  // CPOL = 0
    spi_handle_.Init.CLKPhase = SPI_PHASE_1EDGE;      // CPHA = 0
    spi_handle_.Init.NSS = SPI_NSS_SOFT;
    spi_handle_.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;  // Will be set in begin()
    spi_handle_.Init.FirstBit = SPI_FIRSTBIT_MSB;
    spi_handle_.Init.TIMode = SPI_TIMODE_DISABLE;
    spi_handle_.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    spi_handle_.Init.CRCPolynomial = 7;

#if defined(STM32H7) || defined(STM32H5)
    // Advanced SPI features only available on H7/H5
    spi_handle_.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    spi_handle_.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    spi_handle_.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    spi_handle_.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    spi_handle_.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    spi_handle_.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    spi_handle_.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    spi_handle_.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    spi_handle_.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
    spi_handle_.Init.IOSwap = SPI_IO_SWAP_DISABLE;
#elif defined(STM32G0) || defined(STM32G4) || defined(STM32F4)
    // Simpler SPI on G0/G4/F4 - only has NSSPMode
    spi_handle_.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
#endif
}

void SPI::begin(const uint32_t clock_speed)
{
    // Enable GPIO port clocks
    enableGPIOClock(mosi_port_);
    enableGPIOClock(miso_port_);
    enableGPIOClock(sck_port_);

    // Configure GPIO pins for SPI
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = alternate_function_;

    // Configure MOSI pin
    GPIO_InitStruct.Pin = mosi_pin_;
    HAL_GPIO_Init(mosi_port_, &GPIO_InitStruct);

    // Configure MISO pin
    GPIO_InitStruct.Pin = miso_pin_;
    HAL_GPIO_Init(miso_port_, &GPIO_InitStruct);

    // Configure SCK pin
    GPIO_InitStruct.Pin = sck_pin_;
    HAL_GPIO_Init(sck_port_, &GPIO_InitStruct);

    // Enable SPI peripheral clock
    if (spi_handle_.Instance == SPI1) {
        __HAL_RCC_SPI1_CLK_ENABLE();
    }
#ifdef SPI2
    else if (spi_handle_.Instance == SPI2) {
        __HAL_RCC_SPI2_CLK_ENABLE();
    }
#endif
#ifdef SPI3
    else if (spi_handle_.Instance == SPI3) {
        __HAL_RCC_SPI3_CLK_ENABLE();
    }
#endif
#ifdef SPI4
    else if (spi_handle_.Instance == SPI4) {
        __HAL_RCC_SPI4_CLK_ENABLE();
    }
#endif
#ifdef SPI5
    else if (spi_handle_.Instance == SPI5) {
        __HAL_RCC_SPI5_CLK_ENABLE();
    }
#endif
#ifdef SPI6
    else if (spi_handle_.Instance == SPI6) {
        __HAL_RCC_SPI6_CLK_ENABLE();
    }
#endif

    // Set baud rate prescaler for requested clock speed
    spi_handle_.Init.BaudRatePrescaler = calculatePrescaler(clock_speed);

    // Initialize SPI peripheral
    if (HAL_SPI_Init(&spi_handle_) != HAL_OK)
    {
        // Error handling
        return;
    }
}

void SPI::end()
{
    HAL_SPI_DeInit(&spi_handle_);

    // Deinitialize GPIO pins
    HAL_GPIO_DeInit(mosi_port_, mosi_pin_);
    HAL_GPIO_DeInit(miso_port_, miso_pin_);
    HAL_GPIO_DeInit(sck_port_, sck_pin_);
}

uint32_t SPI::calculatePrescaler(uint32_t clock_speed)
{
    // Assume SPI kernel clock is 100 MHz (typical for STM32H7)
    // Calculate required prescaler
    uint32_t kernel_clock = 100000000;  // 100 MHz
    uint32_t prescaler_value = kernel_clock / clock_speed;

    // Find closest prescaler (2, 4, 8, 16, 32, 64, 128, 256)
    if (prescaler_value <= 2) {
        return SPI_BAUDRATEPRESCALER_2;
    } else if (prescaler_value <= 4) {
        return SPI_BAUDRATEPRESCALER_4;
    } else if (prescaler_value <= 8) {
        return SPI_BAUDRATEPRESCALER_8;
    } else if (prescaler_value <= 16) {
        return SPI_BAUDRATEPRESCALER_16;
    } else if (prescaler_value <= 32) {
        return SPI_BAUDRATEPRESCALER_32;
    } else if (prescaler_value <= 64) {
        return SPI_BAUDRATEPRESCALER_64;
    } else if (prescaler_value <= 128) {
        return SPI_BAUDRATEPRESCALER_128;
    } else {
        return SPI_BAUDRATEPRESCALER_256;
    }
}

bool SPI::transfer(const uint8_t* tx_data, uint8_t* rx_data, uint16_t length, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(
        &spi_handle_,
        (uint8_t*)tx_data,
        rx_data,
        length,
        timeout
    );

    return (status == HAL_OK);
}

uint8_t SPI::transfer(uint8_t data, uint32_t timeout)
{
    uint8_t rx_data = 0;
    transfer(&data, &rx_data, 1, timeout);
    return rx_data;
}

bool SPI::write(const uint8_t* data, uint16_t length, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_SPI_Transmit(
        &spi_handle_,
        (uint8_t*)data,
        length,
        timeout
    );

    return (status == HAL_OK);
}

bool SPI::write(uint8_t data, uint32_t timeout)
{
    return write(&data, 1, timeout);
}

bool SPI::read(uint8_t* data, uint16_t length, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_SPI_Receive(
        &spi_handle_,
        data,
        length,
        timeout
    );

    return (status == HAL_OK);
}

uint8_t SPI::read(uint32_t timeout)
{
    uint8_t data = 0;
    read(&data, 1, timeout);
    return data;
}

bool SPI::isReady()
{
    return (HAL_SPI_GetState(&spi_handle_) == HAL_SPI_STATE_READY);
}

uint32_t SPI::getError()
{
    return HAL_SPI_GetError(&spi_handle_);
}
