#pragma once

// Platform-specific HAL headers
#if defined(STM32H7)
    #include "stm32h7xx_hal.h"
    #include "stm32h7xx_hal_spi.h"
#elif defined(STM32G0)
    #include "stm32g0xx_hal.h"
    #include "stm32g0xx_hal_spi.h"
#elif defined(STM32G4)
    #include "stm32g4xx_hal.h"
    #include "stm32g4xx_hal_spi.h"
#elif defined(STM32F4)
    #include "stm32f4xx_hal.h"
    #include "stm32f4xx_hal_spi.h"
#elif defined(STM32H5)
    #include "stm32h5xx_hal.h"
    #include "stm32h5xx_hal_spi.h"
#else
    #error "Unsupported STM32 platform. Define STM32H7, STM32G0, STM32G4, STM32F4, or STM32H5."
#endif

// SPI Class - Serial Peripheral Interface
// Usage Example:
//   spi1.begin(1000000);  // Start SPI at 1 MHz
//
//   // Transfer data (full duplex)
//   uint8_t tx_data[] = {0x01, 0x02, 0x03};
//   uint8_t rx_data[3];
//   spi1.transfer(tx_data, rx_data, 3);
//
//   // Transmit only
//   spi1.write(tx_data, 3);
//
//   // Receive only
//   spi1.read(rx_data, 3);
//
//   // Single byte transfer
//   uint8_t response = spi1.transfer(0x42);
class SPI
{
private:
    SPI_HandleTypeDef spi_handle_;

    // GPIO configuration
    GPIO_TypeDef* mosi_port_;
    uint16_t mosi_pin_;
    GPIO_TypeDef* miso_port_;
    uint16_t miso_pin_;
    GPIO_TypeDef* sck_port_;
    uint16_t sck_pin_;
    uint32_t alternate_function_;

public:
    SPI() = delete;
    SPI(SPI_TypeDef* spi_instance,
        GPIO_TypeDef* mosi_port, uint16_t mosi_pin,
        GPIO_TypeDef* miso_port, uint16_t miso_pin,
        GPIO_TypeDef* sck_port, uint16_t sck_pin,
        uint32_t alternate_function);

    void begin(const uint32_t clock_speed = 1000000);
    void end();

    // Fluent API setters
    SPI& setMode(const uint32_t mode)
    {
        spi_handle_.Init.Mode = mode;
        HAL_SPI_Init(&spi_handle_);
        return *this;
    }

    SPI& setDataSize(const uint32_t data_size)
    {
        spi_handle_.Init.DataSize = data_size;
        HAL_SPI_Init(&spi_handle_);
        return *this;
    }

    SPI& setCPOL(const uint32_t cpol)
    {
        spi_handle_.Init.CLKPolarity = cpol;
        HAL_SPI_Init(&spi_handle_);
        return *this;
    }

    SPI& setCPHA(const uint32_t cpha)
    {
        spi_handle_.Init.CLKPhase = cpha;
        HAL_SPI_Init(&spi_handle_);
        return *this;
    }

    SPI& setFirstBit(const uint32_t first_bit)
    {
        spi_handle_.Init.FirstBit = first_bit;
        HAL_SPI_Init(&spi_handle_);
        return *this;
    }

    // Full duplex transfer
    bool transfer(const uint8_t* tx_data, uint8_t* rx_data, uint16_t length, uint32_t timeout = 1000);
    uint8_t transfer(uint8_t data, uint32_t timeout = 1000);

    // Transmit only
    bool write(const uint8_t* data, uint16_t length, uint32_t timeout = 1000);
    bool write(uint8_t data, uint32_t timeout = 1000);

    // Receive only
    bool read(uint8_t* data, uint16_t length, uint32_t timeout = 1000);
    uint8_t read(uint32_t timeout = 1000);

    // Status
    bool isReady();
    uint32_t getError();

private:
    uint32_t calculatePrescaler(uint32_t clock_speed);
};
