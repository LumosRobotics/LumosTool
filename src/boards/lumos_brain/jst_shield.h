#pragma once

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_uart.h"

class Serial
{
private:
    UART_HandleTypeDef uart_handle_;

public:
    Serial() = delete;
    // Serial(const int num);
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

extern Serial Serial1;
extern Serial Serial2;

/*#define FDCAN1_TX PD1
#define FDCAN1_RX PD0

#define FDCAN2_TX PB13
#define FDCAN2_RX PB12

#define FDCAN3_TX PD13
#define FDCAN3_RX PD12

#define UART7_TX PE8
#define UART7_RX PE7

#define UART8_TX PE1
#define UART8_RX PE0


//// I2C Pins

#define I2C1_SCL PB6
#define I2C1_SDA PB7

#define I2C4_SCL PB8
#define I2C4_SDA PB9

#define I2C2_SCL PB10
#define I2C2_SDA PB11

SPI2_MISO PC2
SPI2_MOSI PC1
SPI2_SCK PD3

Bottom PB1, PB6
Just above Bottom PB0

*/
