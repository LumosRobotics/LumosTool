#include "lumos_brain.h"

// Create global SD card instance
SDCard sdcard{SDMMC1};

// Create global USB instance
USB usb{USB_OTG_HS};

Serial SerialESP{UART4};
Serial SerialCom{UART6};
