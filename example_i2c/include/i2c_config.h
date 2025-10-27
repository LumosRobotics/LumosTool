#pragma once

#include "stm32h7xx_hal.h"

// I2C Configuration
#define I2C_INSTANCE            I2C1
#define I2C_TIMING              0x60404E72  // 100kHz @ 550MHz CPU, calculated for STM32H7

// I2C GPIO Configuration
// I2C1: PB6 (SCL), PB7 (SDA) - Common on STM32H7 dev boards
#define I2C_SCL_GPIO_PORT       GPIOB
#define I2C_SCL_PIN             GPIO_PIN_6
#define I2C_SCL_AF              GPIO_AF4_I2C1

#define I2C_SDA_GPIO_PORT       GPIOB
#define I2C_SDA_PIN             GPIO_PIN_7
#define I2C_SDA_AF              GPIO_AF4_I2C1

// Function prototypes
void SystemClock_Config(void);
void I2C_Init(void);
HAL_StatusTypeDef I2C_ScanBus(uint8_t* devices, uint8_t* count);
HAL_StatusTypeDef I2C_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);
HAL_StatusTypeDef I2C_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data);
HAL_StatusTypeDef I2C_ReadMulti(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint16_t len);
void Error_Handler(void);
