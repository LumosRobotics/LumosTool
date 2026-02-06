#include "lumos_brain.h"

// Create global SD card instance
// SDMMC1: PD2 (CMD), PC12 (CLK), PC8 (D0), PC9 (D1), PC10 (D2), PC11 (D3)
SDCard sdcard{SDMMC1,
              GPIOD, GPIO_PIN_2,   // CMD
              GPIOC, GPIO_PIN_12,  // CLK
              GPIOC, GPIO_PIN_8,   // D0
              GPIOC, GPIO_PIN_9,   // D1
              GPIOC, GPIO_PIN_10,  // D2
              GPIOC, GPIO_PIN_11,  // D3
              GPIO_AF12_SDMMC1};

// Create global USB instance
// USB_DP (D+) == PA12, USB_DM (D-) == PA11
USB usb{USB_OTG_HS, GPIOA, GPIO_PIN_12, GPIOA, GPIO_PIN_11, GPIO_AF10_OTG1_HS};

// UART4 (ESP32): PA0 (TX), PA1 (RX)
Serial SerialESP{UART4, GPIOA, GPIO_PIN_0, GPIOA, GPIO_PIN_1, GPIO_AF8_UART4};

// USART6 (CP2102): PC6 (TX), PC7 (RX)
Serial SerialCom{USART6, GPIOC, GPIO_PIN_6, GPIOC, GPIO_PIN_7, GPIO_AF7_USART6};
