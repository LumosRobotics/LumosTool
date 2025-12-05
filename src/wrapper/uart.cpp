#include "uart.h"

// Helper function to enable GPIO port clock
static void enableGPIOClock(GPIO_TypeDef* port)
{
    if (port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
    else if (port == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();
    else if (port == GPIOH) __HAL_RCC_GPIOH_CLK_ENABLE();
}

// Helper function to enable UART peripheral clock
static void enableUARTClock(USART_TypeDef* uart)
{
    if (uart == USART1) __HAL_RCC_USART1_CLK_ENABLE();
    else if (uart == USART2) __HAL_RCC_USART2_CLK_ENABLE();
    else if (uart == USART3) __HAL_RCC_USART3_CLK_ENABLE();
    else if (uart == UART4) __HAL_RCC_UART4_CLK_ENABLE();
    else if (uart == UART5) __HAL_RCC_UART5_CLK_ENABLE();
    else if (uart == USART6) __HAL_RCC_USART6_CLK_ENABLE();
    else if (uart == UART7) __HAL_RCC_UART7_CLK_ENABLE();
    else if (uart == UART8) __HAL_RCC_UART8_CLK_ENABLE();
    else if (uart == USART10) __HAL_RCC_USART10_CLK_ENABLE();
}

Serial::Serial(USART_TypeDef* usart_def,
               GPIO_TypeDef* tx_port, uint16_t tx_pin,
               GPIO_TypeDef* rx_port, uint16_t rx_pin,
               uint32_t alternate_function)
    : uart_handle_{},
      tx_port_(tx_port),
      tx_pin_(tx_pin),
      rx_port_(rx_port),
      rx_pin_(rx_pin),
      alternate_function_(alternate_function)
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

    // Enable GPIO port clocks
    enableGPIOClock(tx_port_);
    enableGPIOClock(rx_port_);

    // Configure GPIO pins
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Configure TX pin
    GPIO_InitStruct.Pin = tx_pin_;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = alternate_function_;
    HAL_GPIO_Init(tx_port_, &GPIO_InitStruct);

    // Configure RX pin
    GPIO_InitStruct.Pin = rx_pin_;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = alternate_function_;
    HAL_GPIO_Init(rx_port_, &GPIO_InitStruct);

    // Enable UART peripheral clock
    enableUARTClock(uart_handle_.Instance);

    // Initialize UART
    if (HAL_UART_Init(&uart_handle_) != HAL_OK)
    {
        // Error handling - you may want to add error callback here
        return;
    }
}

void Serial::end()
{
    // Deinitialize UART
    HAL_UART_DeInit(&uart_handle_);

    // Deinitialize GPIO pins
    HAL_GPIO_DeInit(tx_port_, tx_pin_);
    HAL_GPIO_DeInit(rx_port_, rx_pin_);
}
