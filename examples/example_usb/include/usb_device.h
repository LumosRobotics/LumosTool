#pragma once

#include "stm32h7xx_hal.h"
#include "usbd_def.h"

// USB Device handle
extern USBD_HandleTypeDef hUsbDeviceFS;

// Function prototypes
void USB_Device_Init(void);
void SystemClock_Config(void);
void Error_Handler(void);
