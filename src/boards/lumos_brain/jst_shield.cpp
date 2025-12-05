#include "jst_shield.h"

Serial Serial7{UART7};
Serial Serial8{UART8};

// Create global CAN instances
CAN CAN1{FDCAN1};
CAN CAN2{FDCAN2};
CAN CAN3{FDCAN3};

// Create global I2C instances (lowercase names to avoid HAL macro conflicts)
I2C i2c1{I2C1};
I2C i2c2{I2C2};
I2C i2c4{I2C4};
