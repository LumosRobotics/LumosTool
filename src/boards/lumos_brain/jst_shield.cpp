#include "jst_shield.h"

Serial::Serial(USART_TypeDef* usart_def) : uart_handle_{}
{
    // Initialize default values
    uart_handle_.Instance = usart_def;
    uart_handle_.Init.BaudRate = 115200;
    uart_handle_.Init.WordLength = UART_WORDLENGTH_8B;
    uart_handle_.Init.StopBits = UART_STOPBITS_1;
    uart_handle_.Init.Parity = UART_PARITY_NONE;
    uart_handle_.Init.Mode = UART_MODE_TX_RX;
    uart_handle_.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart_handle_.Init.OverSampling = UART_OVERSAMPLING_16;
    uart_handle_.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    uart_handle_.Init.ClockPrescaler = UART_PRESCALER_DIV1;

    uart_handle_.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

}

void Serial::begin(const uint32_t baudrate)
{
    uart_handle_.Init.BaudRate = baudrate;
    // if (HAL_UART_Init(&uart_handle_) != HAL_OK)
    {
        // Error_Handler();
    }
}

void Serial::end()
{
}

Serial Serial1{UART7};
Serial Serial2{UART8};
