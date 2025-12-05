#pragma once

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_i2c.h"

// I2C Class - Inter-Integrated Circuit
// Usage Example:
//   i2c1.begin();  // Start I2C at default 100 kHz
//   i2c1.setClock(400000);  // Set to 400 kHz (Fast Mode)
//
//   uint8_t data[] = {0x10, 0x20};
//   i2c1.write(0x50, data, 2);  // Write to device at address 0x50
//
//   uint8_t buffer[4];
//   i2c1.read(0x50, buffer, 4);  // Read from device
//
//   // Register read/write
//   i2c1.writeRegister(0x50, 0x10, 0xFF);  // Write 0xFF to register 0x10
//   uint8_t value = i2c1.readRegister(0x50, 0x10);  // Read register 0x10
class I2C
{
private:
    I2C_HandleTypeDef i2c_handle_;

public:
    I2C() = delete;
    I2C(I2C_TypeDef* i2c_instance);

    void begin(const uint32_t clock_speed = 100000);
    void end();

    // Fluent API setters
    I2C& setClock(const uint32_t clock_speed)
    {
        i2c_handle_.Init.Timing = calculateTiming(clock_speed);
        HAL_I2C_Init(&i2c_handle_);
        return *this;
    }

    I2C& setAddressingMode(const uint32_t mode)
    {
        i2c_handle_.Init.AddressingMode = mode;
        HAL_I2C_Init(&i2c_handle_);
        return *this;
    }

    // Basic read/write operations
    bool write(uint8_t device_address, const uint8_t* data, uint16_t length, uint32_t timeout = 1000);
    bool read(uint8_t device_address, uint8_t* data, uint16_t length, uint32_t timeout = 1000);

    // Memory/Register operations
    bool writeRegister(uint8_t device_address, uint8_t reg_address, uint8_t value, uint32_t timeout = 1000);
    bool writeRegister16(uint8_t device_address, uint8_t reg_address, uint16_t value, uint32_t timeout = 1000);
    bool readRegister(uint8_t device_address, uint8_t reg_address, uint8_t& value, uint32_t timeout = 1000);
    bool readRegister16(uint8_t device_address, uint8_t reg_address, uint16_t& value, uint32_t timeout = 1000);
    bool readRegisters(uint8_t device_address, uint8_t reg_address, uint8_t* data, uint16_t length, uint32_t timeout = 1000);

    // Device detection
    bool probe(uint8_t device_address, uint32_t timeout = 100);
    void scan(uint8_t* found_addresses, uint8_t& count, uint8_t max_count = 128);

    // Error handling
    uint32_t getError();
    bool isReady();

private:
    uint32_t calculateTiming(uint32_t clock_speed);
};
