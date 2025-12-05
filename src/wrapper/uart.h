#pragma once

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_uart.h"

class Serial
{
private:
    UART_HandleTypeDef uart_handle_;

public:
    Serial() = delete;
    Serial(USART_TypeDef* usart_def);

    void begin(const uint32_t baudrate);
    void end();

    Serial& setParity(const uint32_t Parity)
    {
        uart_handle_.Init.Parity = Parity;
        HAL_UART_Init(&uart_handle_);
        return *this;
    }
};
