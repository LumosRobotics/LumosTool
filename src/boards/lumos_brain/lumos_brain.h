#pragma once

#include "stm32h7xx_hal.h"
#include <usb.h>
#include <uart.h>
#include <sd.h>

/*
SD Card Pin Mapping (SDMMC1):
PD2  == SD_COMMAND (CMD)
PC12 == SD_CLK     (Clock)
PC11 == SD_D3      (Data 3)
PC10 == SD_D2      (Data 2)
PC9  == SD_D1      (Data 1)
PC8  == SD_D0      (Data 0)
*/

// Global SD card instance
extern SDCard sdcard;

// Global UART instance connected to ESP32
// UART4 TX == PA0
// UART4 RX == PA1
/*
Serial connected to CP2102
UART6
PC7 == RX
PC6 == TX
*/
extern Serial SerialESP;
extern Serial SerialCom;

/*
USB DP == PA12
USB DM == PA11
*/


// Global USB instance
extern USB usb;