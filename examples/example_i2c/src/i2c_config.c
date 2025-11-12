#include "i2c_config.h"

// I2C handle
I2C_HandleTypeDef hi2c1;

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
 * @brief I2C Initialization
 * @note Configures I2C1 with 100kHz standard mode
 */
void I2C_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable clocks
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    // Configure GPIO pins for I2C
    // PB6: I2C1_SCL
    // PB7: I2C1_SDA
    GPIO_InitStruct.Pin = I2C_SCL_PIN | I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;          // Open-drain for I2C
    GPIO_InitStruct.Pull = GPIO_PULLUP;              // Pull-up required for I2C
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = I2C_SCL_AF;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Configure I2C parameters
    hi2c1.Instance = I2C_INSTANCE;
    hi2c1.Init.Timing = I2C_TIMING;                  // 100kHz @ 550MHz
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }

    // Configure Analog filter
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    {
        Error_Handler();
    }

    // Configure Digital filter (0 = disabled)
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief Scan I2C bus for devices
 * @param devices Array to store found device addresses
 * @param count Pointer to store number of devices found
 * @return HAL status
 */
HAL_StatusTypeDef I2C_ScanBus(uint8_t* devices, uint8_t* count)
{
    HAL_StatusTypeDef result;
    *count = 0;

    // Scan all 7-bit addresses (0x03 to 0x77)
    for (uint8_t addr = 0x03; addr < 0x78; addr++)
    {
        // Try to communicate with device
        result = HAL_I2C_IsDeviceReady(&hi2c1, (addr << 1), 1, 10);

        if (result == HAL_OK)
        {
            devices[*count] = addr;
            (*count)++;
        }
    }

    return HAL_OK;
}

/**
 * @brief Write a byte to a register
 * @param dev_addr Device address (7-bit)
 * @param reg_addr Register address
 * @param data Data to write
 * @return HAL status
 */
HAL_StatusTypeDef I2C_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    uint8_t buf[2];
    buf[0] = reg_addr;
    buf[1] = data;

    return HAL_I2C_Master_Transmit(&hi2c1, (dev_addr << 1), buf, 2, HAL_MAX_DELAY);
}

/**
 * @brief Read a byte from a register
 * @param dev_addr Device address (7-bit)
 * @param reg_addr Register address
 * @param data Pointer to store read data
 * @return HAL status
 */
HAL_StatusTypeDef I2C_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data)
{
    HAL_StatusTypeDef result;

    // Write register address
    result = HAL_I2C_Master_Transmit(&hi2c1, (dev_addr << 1), &reg_addr, 1, HAL_MAX_DELAY);
    if (result != HAL_OK) return result;

    // Read data
    return HAL_I2C_Master_Receive(&hi2c1, (dev_addr << 1), data, 1, HAL_MAX_DELAY);
}

/**
 * @brief Read multiple bytes from a register
 * @param dev_addr Device address (7-bit)
 * @param reg_addr Register address
 * @param data Pointer to store read data
 * @param len Number of bytes to read
 * @return HAL status
 */
HAL_StatusTypeDef I2C_ReadMulti(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint16_t len)
{
    HAL_StatusTypeDef result;

    // Write register address
    result = HAL_I2C_Master_Transmit(&hi2c1, (dev_addr << 1), &reg_addr, 1, HAL_MAX_DELAY);
    if (result != HAL_OK) return result;

    // Read data
    return HAL_I2C_Master_Receive(&hi2c1, (dev_addr << 1), data, len, HAL_MAX_DELAY);
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
