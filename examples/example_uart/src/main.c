#include "uart_config.h"
#include <stdio.h>

// Buffer for sprintf
static char msg_buffer[128];

int main(void)
{
    uint32_t counter = 0;

    // Initialize HAL Library
    HAL_Init();

    // Configure system clock to 550 MHz
    SystemClock_Config();

    // Initialize UART
    UART_Init();

    // Send startup message
    UART_SendString("\r\n");
    UART_SendString("========================================\r\n");
    UART_SendString("  STM32H7 UART Example\r\n");
    UART_SendString("  Lumos Build Tool Demo\r\n");
    UART_SendString("========================================\r\n");
    UART_SendString("\r\n");

    snprintf(msg_buffer, sizeof(msg_buffer),
             "System Clock: %lu MHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000);
    UART_SendString(msg_buffer);

    snprintf(msg_buffer, sizeof(msg_buffer),
             "HCLK: %lu MHz\r\n", HAL_RCC_GetHCLKFreq() / 1000000);
    UART_SendString(msg_buffer);

    snprintf(msg_buffer, sizeof(msg_buffer),
             "UART Baudrate: %lu bps\r\n\r\n", (uint32_t)UART_BAUDRATE);
    UART_SendString(msg_buffer);

    UART_SendString("Starting counter...\r\n\r\n");

    // Main loop
    while (1)
    {
        // Send counter value
        snprintf(msg_buffer, sizeof(msg_buffer),
                 "[%6lu] Hello from STM32H7! System running at %lu MHz\r\n",
                 counter++, HAL_RCC_GetSysClockFreq() / 1000000);
        UART_SendString(msg_buffer);

        // Delay 1 second
        HAL_Delay(1000);
    }

    return 0;
}

// SysTick interrupt handler (required by HAL_Delay)
void SysTick_Handler(void)
{
    HAL_IncTick();
}
