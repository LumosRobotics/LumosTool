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
#ifdef GPIOG
    else if (port == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();
#endif
#ifdef GPIOH
    else if (port == GPIOH) __HAL_RCC_GPIOH_CLK_ENABLE();
#endif
}

// Helper function to enable UART peripheral clock
static void enableUARTClock(USART_TypeDef* uart)
{
#ifdef USART1
    if (uart == USART1) __HAL_RCC_USART1_CLK_ENABLE();
#endif
#ifdef USART2
    else if (uart == USART2) __HAL_RCC_USART2_CLK_ENABLE();
#endif
#ifdef USART3
    else if (uart == USART3) __HAL_RCC_USART3_CLK_ENABLE();
#endif
#ifdef UART4
    else if (uart == UART4) __HAL_RCC_UART4_CLK_ENABLE();
#endif
#ifdef UART5
    else if (uart == UART5) __HAL_RCC_UART5_CLK_ENABLE();
#endif
#ifdef USART4
    else if (uart == USART4) __HAL_RCC_USART4_CLK_ENABLE();
#endif
#ifdef USART5
    else if (uart == USART5) __HAL_RCC_USART5_CLK_ENABLE();
#endif
#ifdef USART6
    else if (uart == USART6) __HAL_RCC_USART6_CLK_ENABLE();
#endif
#ifdef UART7
    else if (uart == UART7) __HAL_RCC_UART7_CLK_ENABLE();
#endif
#ifdef UART8
    else if (uart == UART8) __HAL_RCC_UART8_CLK_ENABLE();
#endif
#ifdef USART10
    else if (uart == USART10) __HAL_RCC_USART10_CLK_ENABLE();
#endif
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

// ===== Data Transmission Methods =====

bool Serial::write(const uint8_t* data, uint16_t length, uint32_t timeout)
{
    if (HAL_UART_Transmit(&uart_handle_, (uint8_t*)data, length, timeout) == HAL_OK) {
        return true;
    }
    return false;
}

bool Serial::write(uint8_t byte)
{
    return write(&byte, 1, 100);
}

// ===== Formatted Print Methods =====

bool Serial::print(const char* str)
{
    if (str == nullptr) return false;
    return write((const uint8_t*)str, strlen(str), 1000);
}

bool Serial::print(int value, int base)
{
    char buffer[34];  // Enough for 32-bit int in binary + null

    if (base == 10) {
        snprintf(buffer, sizeof(buffer), "%d", value);
    } else if (base == 16) {
        snprintf(buffer, sizeof(buffer), "%x", value);
    } else if (base == 8) {
        snprintf(buffer, sizeof(buffer), "%o", value);
    } else if (base == 2) {
        // Binary conversion
        int i = 0;
        unsigned int v = (value < 0) ? -value : value;
        if (value < 0) buffer[i++] = '-';

        // Find first set bit
        int bit_pos = 31;
        while (bit_pos >= 0 && !(v & (1U << bit_pos))) bit_pos--;

        if (bit_pos < 0) {
            buffer[i++] = '0';
        } else {
            for (; bit_pos >= 0; bit_pos--) {
                buffer[i++] = (v & (1U << bit_pos)) ? '1' : '0';
            }
        }
        buffer[i] = '\0';
    } else {
        return false;  // Unsupported base
    }

    return print(buffer);
}

bool Serial::print(unsigned int value, int base)
{
    char buffer[34];

    if (base == 10) {
        snprintf(buffer, sizeof(buffer), "%u", value);
    } else if (base == 16) {
        snprintf(buffer, sizeof(buffer), "%x", value);
    } else if (base == 8) {
        snprintf(buffer, sizeof(buffer), "%o", value);
    } else if (base == 2) {
        // Binary conversion
        int i = 0;
        int bit_pos = 31;
        while (bit_pos >= 0 && !(value & (1U << bit_pos))) bit_pos--;

        if (bit_pos < 0) {
            buffer[i++] = '0';
        } else {
            for (; bit_pos >= 0; bit_pos--) {
                buffer[i++] = (value & (1U << bit_pos)) ? '1' : '0';
            }
        }
        buffer[i] = '\0';
    } else {
        return false;
    }

    return print(buffer);
}

bool Serial::print(long value, int base)
{
    return print((int)value, base);
}

bool Serial::print(unsigned long value, int base)
{
    return print((unsigned int)value, base);
}

bool Serial::print(float value, int decimals)
{
    char buffer[32];

    if (decimals == 0) {
        snprintf(buffer, sizeof(buffer), "%.0f", value);
    } else if (decimals == 1) {
        snprintf(buffer, sizeof(buffer), "%.1f", value);
    } else if (decimals == 2) {
        snprintf(buffer, sizeof(buffer), "%.2f", value);
    } else if (decimals == 3) {
        snprintf(buffer, sizeof(buffer), "%.3f", value);
    } else if (decimals == 4) {
        snprintf(buffer, sizeof(buffer), "%.4f", value);
    } else {
        snprintf(buffer, sizeof(buffer), "%f", value);
    }

    return print(buffer);
}

bool Serial::println(const char* str)
{
    if (!print(str)) return false;
    return print("\r\n");
}

bool Serial::println(int value, int base)
{
    if (!print(value, base)) return false;
    return print("\r\n");
}

bool Serial::println(unsigned int value, int base)
{
    if (!print(value, base)) return false;
    return print("\r\n");
}

bool Serial::println(long value, int base)
{
    if (!print(value, base)) return false;
    return print("\r\n");
}

bool Serial::println(unsigned long value, int base)
{
    if (!print(value, base)) return false;
    return print("\r\n");
}

bool Serial::println(float value, int decimals)
{
    if (!print(value, decimals)) return false;
    return print("\r\n");
}

bool Serial::println()
{
    return print("\r\n");
}

// ===== Data Reception Methods =====

uint16_t Serial::available()
{
    // For basic implementation, return 0
    // A full implementation would use interrupt-driven RX with a ring buffer
    return 0;
}

uint16_t Serial::read(uint8_t* buffer, uint16_t length)
{
    if (HAL_UART_Receive(&uart_handle_, buffer, length, 100) == HAL_OK) {
        return length;
    }
    return 0;
}

int Serial::read()
{
    uint8_t byte;
    if (HAL_UART_Receive(&uart_handle_, &byte, 1, 10) == HAL_OK) {
        return byte;
    }
    return -1;
}
