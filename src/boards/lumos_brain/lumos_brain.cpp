#include "lumos_brain.h"

// Create global SD card instance
SDCard sdcard{SDMMC1};

// Create global USB instance
USB usb{USB_OTG_HS};

// UART4 (ESP32): PA0 (TX), PA1 (RX)
Serial SerialESP{UART4, GPIOA, GPIO_PIN_0, GPIOA, GPIO_PIN_1, GPIO_AF8_UART4};

// USART6 (CP2102): PC6 (TX), PC7 (RX)
Serial SerialCom{USART6, GPIOC, GPIO_PIN_6, GPIOC, GPIO_PIN_7, GPIO_AF7_USART6};
