#include "jst_shield.h"

// UART7: PE8 (TX), PE7 (RX)
Serial Serial7{UART7, GPIOE, GPIO_PIN_8, GPIOE, GPIO_PIN_7, GPIO_AF7_UART7};

// UART8: PE1 (TX), PE0 (RX)
Serial Serial8{UART8, GPIOE, GPIO_PIN_1, GPIOE, GPIO_PIN_0, GPIO_AF8_UART8};

// Create global CAN instances
CAN CAN1{FDCAN1};
CAN CAN2{FDCAN2};
CAN CAN3{FDCAN3};

// Create global I2C instances (lowercase names to avoid HAL macro conflicts)
I2C i2c1{I2C1};
I2C i2c2{I2C2};
I2C i2c4{I2C4};
