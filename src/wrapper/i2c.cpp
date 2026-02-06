#include "i2c.h"

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

I2C::I2C(I2C_TypeDef* i2c_instance,
         GPIO_TypeDef* scl_port, uint16_t scl_pin,
         GPIO_TypeDef* sda_port, uint16_t sda_pin,
         uint32_t alternate_function)
    : i2c_handle_{},
      scl_port_(scl_port),
      scl_pin_(scl_pin),
      sda_port_(sda_port),
      sda_pin_(sda_pin),
      alternate_function_(alternate_function)
{
    // Initialize I2C handle with default values
    i2c_handle_.Instance = i2c_instance;

    // Default timing for 100 kHz (Standard Mode)
    i2c_handle_.Init.Timing = calculateTiming(100000);
    i2c_handle_.Init.OwnAddress1 = 0;
    i2c_handle_.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    i2c_handle_.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    i2c_handle_.Init.OwnAddress2 = 0;
    i2c_handle_.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    i2c_handle_.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    i2c_handle_.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
}

void I2C::begin(const uint32_t clock_speed)
{
    // Enable GPIO port clocks
    enableGPIOClock(scl_port_);
    enableGPIOClock(sda_port_);

    // Configure GPIO pins for I2C
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;  // Open-drain for I2C
    GPIO_InitStruct.Pull = GPIO_PULLUP;       // Pull-up required for I2C
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = alternate_function_;

    // Configure SCL pin
    GPIO_InitStruct.Pin = scl_pin_;
    HAL_GPIO_Init(scl_port_, &GPIO_InitStruct);

    // Configure SDA pin
    GPIO_InitStruct.Pin = sda_pin_;
    HAL_GPIO_Init(sda_port_, &GPIO_InitStruct);

    // Enable I2C peripheral clock
    if (i2c_handle_.Instance == I2C1) {
        __HAL_RCC_I2C1_CLK_ENABLE();
    }
#ifdef I2C2
    else if (i2c_handle_.Instance == I2C2) {
        __HAL_RCC_I2C2_CLK_ENABLE();
    }
#endif
#ifdef I2C3
    else if (i2c_handle_.Instance == I2C3) {
        __HAL_RCC_I2C3_CLK_ENABLE();
    }
#endif
#ifdef I2C4
    else if (i2c_handle_.Instance == I2C4) {
        __HAL_RCC_I2C4_CLK_ENABLE();
    }
#endif

    // Set timing for requested clock speed
    i2c_handle_.Init.Timing = calculateTiming(clock_speed);

    // Initialize I2C peripheral
    if (HAL_I2C_Init(&i2c_handle_) != HAL_OK)
    {
        // Error handling
        return;
    }

    // Configure analog filter
    HAL_I2CEx_ConfigAnalogFilter(&i2c_handle_, I2C_ANALOGFILTER_ENABLE);
}

void I2C::end()
{
    HAL_I2C_DeInit(&i2c_handle_);

    // Deinitialize GPIO pins
    HAL_GPIO_DeInit(scl_port_, scl_pin_);
    HAL_GPIO_DeInit(sda_port_, sda_pin_);
}

uint32_t I2C::calculateTiming(uint32_t clock_speed)
{
    // Timing calculation for STM32H7 at 100 MHz I2C kernel clock
    // These values are precalculated for common speeds
    // For precise calculation, use STM32CubeMX I2C timing calculator

    if (clock_speed <= 100000) {
        // Standard Mode (100 kHz)
        // Timing: PRESC=15, SCLDEL=4, SDADEL=2, SCLH=0xF3, SCLL=0xF9
        return 0xF0420F13;  // 100 kHz
    }
    else if (clock_speed <= 400000) {
        // Fast Mode (400 kHz)
        // Timing: PRESC=3, SCLDEL=3, SDADEL=1, SCLH=0x3C, SCLL=0x49
        return 0x30B0364D;  // 400 kHz
    }
    else {
        // Fast Mode Plus (1 MHz)
        // Timing: PRESC=1, SCLDEL=4, SDADEL=0, SCLH=0x0F, SCLL=0x13
        return 0x10400413;  // 1 MHz
    }
}

bool I2C::write(uint8_t device_address, const uint8_t* data, uint16_t length, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        &i2c_handle_,
        device_address << 1,  // Shift address for HAL
        (uint8_t*)data,
        length,
        timeout
    );

    return (status == HAL_OK);
}

bool I2C::read(uint8_t device_address, uint8_t* data, uint16_t length, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_I2C_Master_Receive(
        &i2c_handle_,
        device_address << 1,  // Shift address for HAL
        data,
        length,
        timeout
    );

    return (status == HAL_OK);
}

bool I2C::writeRegister(uint8_t device_address, uint8_t reg_address, uint8_t value, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(
        &i2c_handle_,
        device_address << 1,
        reg_address,
        I2C_MEMADD_SIZE_8BIT,
        &value,
        1,
        timeout
    );

    return (status == HAL_OK);
}

bool I2C::writeRegister16(uint8_t device_address, uint8_t reg_address, uint16_t value, uint32_t timeout)
{
    uint8_t data[2];
    data[0] = (value >> 8) & 0xFF;  // MSB
    data[1] = value & 0xFF;          // LSB

    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(
        &i2c_handle_,
        device_address << 1,
        reg_address,
        I2C_MEMADD_SIZE_8BIT,
        data,
        2,
        timeout
    );

    return (status == HAL_OK);
}

bool I2C::readRegister(uint8_t device_address, uint8_t reg_address, uint8_t& value, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        &i2c_handle_,
        device_address << 1,
        reg_address,
        I2C_MEMADD_SIZE_8BIT,
        &value,
        1,
        timeout
    );

    return (status == HAL_OK);
}

bool I2C::readRegister16(uint8_t device_address, uint8_t reg_address, uint16_t& value, uint32_t timeout)
{
    uint8_t data[2];

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        &i2c_handle_,
        device_address << 1,
        reg_address,
        I2C_MEMADD_SIZE_8BIT,
        data,
        2,
        timeout
    );

    if (status == HAL_OK)
    {
        value = (data[0] << 8) | data[1];  // MSB first
        return true;
    }

    return false;
}

bool I2C::readRegisters(uint8_t device_address, uint8_t reg_address, uint8_t* data, uint16_t length, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        &i2c_handle_,
        device_address << 1,
        reg_address,
        I2C_MEMADD_SIZE_8BIT,
        data,
        length,
        timeout
    );

    return (status == HAL_OK);
}

bool I2C::probe(uint8_t device_address, uint32_t timeout)
{
    HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(
        &i2c_handle_,
        device_address << 1,
        1,  // Number of trials
        timeout
    );

    return (status == HAL_OK);
}

void I2C::scan(uint8_t* found_addresses, uint8_t& count, uint8_t max_count)
{
    count = 0;

    // Scan all 7-bit addresses (0x08 to 0x77)
    for (uint8_t addr = 0x08; addr < 0x78; addr++)
    {
        if (count >= max_count)
            break;

        if (probe(addr, 10))  // 10ms timeout for scan
        {
            found_addresses[count++] = addr;
        }
    }
}

uint32_t I2C::getError()
{
    return HAL_I2C_GetError(&i2c_handle_);
}

bool I2C::isReady()
{
    return (HAL_I2C_GetState(&i2c_handle_) == HAL_I2C_STATE_READY);
}
