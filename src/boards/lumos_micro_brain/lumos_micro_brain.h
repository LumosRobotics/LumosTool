#pragma once

#include "stm32g0xx_hal.h"
#include <uart.h>
#include <spi.h>
#include <i2c.h>
#include <can.h>

/*
================================
 LumosMicroBrain Pin Mappings
================================
MCU: STM32G0B1CBU3
CPU: ARM Cortex-M0+ @ 64 MHz
Flash: 128 KB
RAM: 144 KB

NOTE: Some pins are shared between peripherals. Use only one peripheral at a time
on shared pins. For example, JST connector pins PB8/PB9 can be used for:
- UART3 (Serial communication)
- UART6 (Serial communication)
- I2C1 (I2C communication)
Choose the peripheral that matches your application needs.

UART Mappings:
--------------
UART1 (Programming Port): PA9 (TX), PA10 (RX)
UART3 (JST Connector):    PB8 (TX), PB9 (RX)  [Shares pins with I2C1, UART6]
UART5 (Bottom Connector): PB2 (TX), PB1 (RX)
UART6 (JST Connector):    PB8 (TX), PB9 (RX)  [Shares pins with I2C1, UART3]

I2C Mappings:
-------------
I2C1 (JST Connector):     PB8 (SCL), PB9 (SDA)  [Shares pins with UART3, UART6]
I2C3 (Bottom Connector):  PA7 (SCL), PA6 (SDA)

SPI Mappings:
-------------
SPI3 (Bottom Connector):  PB3 (SCK), PB4 (MISO), PB5 (MOSI)

CAN Mapping:
------------
FDCAN1: PB11/PA9 (TX), PB12/PA10 (RX)  [Note: Can use alternate pins]

GPIO Pins (Bottom Connector):
------------------------------
PA15 (pin 2), PD2 (pin 4), PA2 (pin 13), PA1 (pin 15), PB7 (pin 16)
*/

// ===========================
// UART Instances
// ===========================
extern Serial SerialPgm;    // UART1 - Programming port (PA9, PA10)
extern Serial SerialJst;    // UART3 - JST connector (PB8, PB9)
extern Serial SerialBottom; // UART5 - Bottom connector (PB2, PB1)

// ===========================
// I2C Instances
// ===========================
extern I2C I2C_Jst;         // I2C1 - JST connector (PB8 SCL, PB9 SDA)
extern I2C I2C_Bottom;      // I2C3 - Bottom connector (PA7 SCL, PA6 SDA)

// ===========================
// SPI Instances
// ===========================
extern SPI SPI_Bottom;      // SPI3 - Bottom connector (PB3 SCK, PB4 MISO, PB5 MOSI)

// ===========================
// CAN Instance
// ===========================
extern CAN CAN1;            // FDCAN1 - CAN bus (PB11 TX, PB12 RX)

