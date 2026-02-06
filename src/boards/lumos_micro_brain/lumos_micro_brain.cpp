#include "lumos_micro_brain.h"

// ===========================
// UART Instances
// ===========================

// USART1 (Programming Port): PA9 (TX), PA10 (RX)
Serial SerialPgm{USART1, GPIOA, GPIO_PIN_9, GPIOA, GPIO_PIN_10, GPIO_AF1_USART1};

// USART3 (JST Connector): PB8 (TX), PB9 (RX)
// Note: Shares pins with I2C1. Use only one peripheral at a time.
Serial SerialJst{USART3, GPIOB, GPIO_PIN_8, GPIOB, GPIO_PIN_9, GPIO_AF4_USART3};

// USART5 (Bottom Connector): PB2 (TX), PB1 (RX)
Serial SerialBottom{USART5, GPIOB, GPIO_PIN_2, GPIOB, GPIO_PIN_1, GPIO_AF3_USART5};

// ===========================
// I2C Instances
// ===========================

// I2C1 (JST Connector): PB8 (SCL), PB9 (SDA)
// Note: Shares pins with UART3/UART6. Use only one peripheral at a time.
I2C I2C_Jst{I2C1, GPIOB, GPIO_PIN_8, GPIOB, GPIO_PIN_9, GPIO_AF6_I2C1};

// I2C3 (Bottom Connector): PA7 (SCL), PA6 (SDA)
I2C I2C_Bottom{I2C3, GPIOA, GPIO_PIN_7, GPIOA, GPIO_PIN_6, GPIO_AF6_I2C3};

// ===========================
// SPI Instances
// ===========================

// SPI3 (Bottom Connector): PB3 (SCK), PB4 (MISO), PB5 (MOSI)
SPI SPI_Bottom{SPI3, GPIOB, GPIO_PIN_5, GPIOB, GPIO_PIN_4, GPIOB, GPIO_PIN_3, GPIO_AF9_SPI3};

// ===========================
// CAN Instance
// ===========================

// FDCAN1: PB11 (TX), PB12 (RX)
// Note: Alternate pins PA9 (TX), PA10 (RX) are available but share with UART1
CAN CAN1{FDCAN1, GPIOB, GPIO_PIN_11, GPIOB, GPIO_PIN_12, GPIO_AF3_FDCAN1};