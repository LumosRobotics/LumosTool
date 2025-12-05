#pragma once

#include "stm32h7xx_hal.h"
#include <uart.h>
#include <i2c.h>
#include <can.h>

extern Serial Serial7;
extern Serial Serial8;

extern CAN CAN1;
extern CAN CAN2;
extern CAN CAN3;

// Using lowercase names to avoid conflict with HAL I2C1/I2C2/I2C4 macros
extern I2C i2c1;
extern I2C i2c2;
extern I2C i2c4;

/*
SpiPort Spi1
Spi1.begin(200000r);
Spi1.

Always same "extra" pin for SS
For "extended" port, always Pin furthest out that is GP


*/

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
