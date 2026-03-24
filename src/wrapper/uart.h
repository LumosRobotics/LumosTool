#pragma once

// Platform-specific HAL headers
// The board header must include the appropriate STM32 HAL before including this file
// For example: stm32h7xx_hal.h, stm32g0xx_hal.h, stm32f4xx_hal.h, etc.
#if defined(STM32H7)
    #include "stm32h7xx_hal.h"
    #include "stm32h7xx_hal_uart.h"
#elif defined(STM32G0)
    #include "stm32g0xx_hal.h"
    #include "stm32g0xx_hal_uart.h"
#elif defined(STM32G4)
    #include "stm32g4xx_hal.h"
    #include "stm32g4xx_hal_uart.h"
#elif defined(STM32F4)
    #include "stm32f4xx_hal.h"
    #include "stm32f4xx_hal_uart.h"
#elif defined(STM32H5)
    #include "stm32h5xx_hal.h"
    #include "stm32h5xx_hal_uart.h"
#else
    #error "Unsupported STM32 platform. Define STM32H7, STM32G0, STM32G4, STM32F4, or STM32H5."
#endif

#include <cstring>
#include <cstdio>

class Serial
{
private:
    UART_HandleTypeDef uart_handle_;
    GPIO_TypeDef* tx_port_;
    uint16_t tx_pin_;
    GPIO_TypeDef* rx_port_;
    uint16_t rx_pin_;
    uint32_t alternate_function_;

public:
    Serial() = delete;
    Serial(USART_TypeDef* usart_def,
           GPIO_TypeDef* tx_port, uint16_t tx_pin,
           GPIO_TypeDef* rx_port, uint16_t rx_pin,
           uint32_t alternate_function);

    void begin(const uint32_t baudrate = 115200);
    void end();

    Serial& setParity(const uint32_t Parity)
    {
        uart_handle_.Init.Parity = Parity;
        HAL_UART_Init(&uart_handle_);
        return *this;
    }

    // Data transmission
    bool write(const uint8_t* data, uint16_t length, uint32_t timeout = 100);
    bool write(uint8_t byte);

    // Formatted output (like Arduino Serial)
    bool print(const char* str);
    bool print(int value, int base = 10);
    bool print(unsigned int value, int base = 10);
    bool print(long value, int base = 10);
    bool print(unsigned long value, int base = 10);
    bool print(float value, int decimals = 2);
    bool println(const char* str);
    bool println(int value, int base = 10);
    bool println(unsigned int value, int base = 10);
    bool println(long value, int base = 10);
    bool println(unsigned long value, int base = 10);
    bool println(float value, int decimals = 2);
    bool println();  // Just newline

    // Data reception
    uint16_t available();
    uint16_t read(uint8_t* buffer, uint16_t length);
    int read();  // Read single byte, returns -1 if no data
};
