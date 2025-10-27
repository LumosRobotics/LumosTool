#pragma once

#include "stm32h7xx_hal.h"

// SPI Configuration
#define SPI_INSTANCE            SPI1

// SPI GPIO Configuration
// SPI1: PA5 (SCK), PA6 (MISO), PA7 (MOSI) - Common on STM32H7 dev boards
#define SPI_SCK_GPIO_PORT       GPIOA
#define SPI_SCK_PIN             GPIO_PIN_5
#define SPI_SCK_AF              GPIO_AF5_SPI1

#define SPI_MISO_GPIO_PORT      GPIOA
#define SPI_MISO_PIN            GPIO_PIN_6
#define SPI_MISO_AF             GPIO_AF5_SPI1

#define SPI_MOSI_GPIO_PORT      GPIOA
#define SPI_MOSI_PIN            GPIO_PIN_7
#define SPI_MOSI_AF             GPIO_AF5_SPI1

// Chip Select (CS) GPIO - Software controlled
#define SPI_CS_GPIO_PORT        GPIOA
#define SPI_CS_PIN              GPIO_PIN_4

// SPI CS Control Macros
#define SPI_CS_LOW()            HAL_GPIO_WritePin(SPI_CS_GPIO_PORT, SPI_CS_PIN, GPIO_PIN_RESET)
#define SPI_CS_HIGH()           HAL_GPIO_WritePin(SPI_CS_GPIO_PORT, SPI_CS_PIN, GPIO_PIN_SET)

// Function prototypes
void SystemClock_Config(void);
void SPI_Init(void);
HAL_StatusTypeDef SPI_Transmit(uint8_t* data, uint16_t size);
HAL_StatusTypeDef SPI_Receive(uint8_t* data, uint16_t size);
HAL_StatusTypeDef SPI_TransmitReceive(uint8_t* tx_data, uint8_t* rx_data, uint16_t size);
uint8_t SPI_TransferByte(uint8_t data);
void Error_Handler(void);
