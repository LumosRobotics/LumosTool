#pragma once

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_uart.h"

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
};
