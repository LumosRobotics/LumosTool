#include "spi_config.h"

// SPI handle
SPI_HandleTypeDef hspi1;

/**
 * @brief System Clock Configuration
 * @note Configures the system to run at 550 MHz (max for H7)
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Supply configuration update enable
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

    // Configure the main internal regulator output voltage
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    // Configure HSE (High Speed External oscillator)
    // Assuming 25 MHz external crystal (common on H7 boards)
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

    // PLL1 configuration for 550 MHz
    // HSE = 25 MHz
    // VCO = (HSE / PLLM) * PLLN = (25 / 5) * 220 = 1100 MHz
    // SYSCLK = VCO / PLLP = 1100 / 2 = 550 MHz
    RCC_OscInitStruct.PLL.PLLM = 5;
    RCC_OscInitStruct.PLL.PLLN = 220;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    // Configure CPU, AHB and APB buses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                  RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;      // 275 MHz
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;     // 137.5 MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;     // 137.5 MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;     // 137.5 MHz
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;     // 137.5 MHz

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief SPI Initialization
 * @note Configures SPI1 in master mode, full duplex, 8-bit data
 * @note Uses low speed initially for SD card initialization (400 kHz)
 */
void SPI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    // Configure GPIO pins for SPI
    // PA5: SPI1_SCK
    // PA6: SPI1_MISO
    // PA7: SPI1_MOSI
    GPIO_InitStruct.Pin = SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = SPI_SCK_AF;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure CS pin as GPIO output
    GPIO_InitStruct.Pin = SPI_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(SPI_CS_GPIO_PORT, &GPIO_InitStruct);

    // CS idle high
    SPI_CS_HIGH();

    // Configure SPI parameters
    // Start with low speed for SD card initialization (400 kHz required by SD spec)
    hspi1.Instance = SPI_INSTANCE;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;       // Full duplex
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;         // CPOL = 0
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;             // CPHA = 0 (Mode 0)
    hspi1.Init.NSS = SPI_NSS_SOFT;                     // Software CS control
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;  // ~537 kHz (137.5 MHz / 256)
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 0x0;
    hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;

    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief Transmit data over SPI
 * @param data Pointer to data buffer
 * @param size Number of bytes to transmit
 * @return HAL status
 */
HAL_StatusTypeDef SPI_Transmit(uint8_t* data, uint16_t size)
{
    return HAL_SPI_Transmit(&hspi1, data, size, HAL_MAX_DELAY);
}

/**
 * @brief Receive data over SPI
 * @param data Pointer to receive buffer
 * @param size Number of bytes to receive
 * @return HAL status
 */
HAL_StatusTypeDef SPI_Receive(uint8_t* data, uint16_t size)
{
    return HAL_SPI_Receive(&hspi1, data, size, HAL_MAX_DELAY);
}

/**
 * @brief Transmit and receive data over SPI (full duplex)
 * @param tx_data Pointer to transmit data buffer
 * @param rx_data Pointer to receive data buffer
 * @param size Number of bytes to transfer
 * @return HAL status
 */
HAL_StatusTypeDef SPI_TransmitReceive(uint8_t* tx_data, uint8_t* rx_data, uint16_t size)
{
    return HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, size, HAL_MAX_DELAY);
}

/**
 * @brief Transfer a single byte (transmit and receive)
 * @param data Byte to transmit
 * @return Received byte
 */
uint8_t SPI_TransferByte(uint8_t data)
{
    uint8_t rx_data;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rx_data, 1, HAL_MAX_DELAY);
    return rx_data;
}

/**
 * @brief Error handler - infinite loop
 */
void Error_Handler(void)
{
    // Disable interrupts
    __disable_irq();

    // Infinite loop
    while (1)
    {
        // Optionally toggle an LED here if available
    }
}
