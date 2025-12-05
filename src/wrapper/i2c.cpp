#include "i2c.h"

I2C::I2C(I2C_TypeDef* i2c_instance) : i2c_handle_{}
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
