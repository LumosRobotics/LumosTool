#pragma once

#include "stm32h7xx_hal.h"

// UART Configuration
#define UART_INSTANCE           USART3
#define UART_BAUDRATE           115200

// UART GPIO Configuration
// USART3: PD8 (TX), PD9 (RX) - Common on STM32H7 dev boards
#define UART_TX_GPIO_PORT       GPIOD
#define UART_TX_PIN             GPIO_PIN_8
#define UART_TX_AF              GPIO_AF7_USART3

#define UART_RX_GPIO_PORT       GPIOD
#define UART_RX_PIN             GPIO_PIN_9
#define UART_RX_AF              GPIO_AF7_USART3

// Function prototypes
void SystemClock_Config(void);
void UART_Init(void);
void UART_SendString(const char* str);
void Error_Handler(void);
