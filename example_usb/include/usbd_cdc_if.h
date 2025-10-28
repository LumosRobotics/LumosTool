#pragma once

#include "usbd_cdc.h"

// CDC Interface callback structure
extern USBD_CDC_ItfTypeDef USBD_Interface_fops_FS;

// CDC transmit functions
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
