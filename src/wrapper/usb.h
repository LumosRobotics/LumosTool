#pragma once

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_pcd.h"
#include <cstring>
#include <cstdio>

// USB Class - USB OTG High Speed with CDC (Virtual COM Port)
// Usage Example:
//   usb.begin();  // Initialize USB CDC
//
//   // Send data
//   usb.write((uint8_t*)"Hello", 5);
//   usb.print("Temperature: ");
//   usb.println(25.5);
//
//   // Receive data
//   if (usb.available()) {
//       uint8_t buffer[64];
//       uint16_t len = usb.read(buffer, sizeof(buffer));
//   }
//
//   // Check connection status
//   if (usb.isConnected()) {
//       // USB host is connected
//   }
class USB
{
private:
    static constexpr uint16_t RX_BUFFER_SIZE = 1024;

    PCD_HandleTypeDef pcd_handle_;
    uint8_t rx_buffer_[RX_BUFFER_SIZE];
    volatile uint16_t rx_head_;
    volatile uint16_t rx_tail_;
    volatile bool initialized_;
    volatile bool connected_;

public:
    USB() = delete;
    USB(PCD_TypeDef* usb_instance);

    // Initialization
    bool begin();
    void end();

    // Data transmission
    bool write(const uint8_t* data, uint16_t length, uint32_t timeout = 100);
    bool write(uint8_t byte);

    // Formatted output (like Serial)
    bool print(const char* str);
    bool print(int value);
    bool print(float value, int decimals = 2);
    bool println(const char* str);
    bool println(int value);
    bool println(float value, int decimals = 2);
    bool println();  // Just newline

    // Data reception
    uint16_t available();
    uint16_t read(uint8_t* buffer, uint16_t length);
    int read();  // Read single byte, returns -1 if no data

    // Status
    bool isConnected();
    bool isReady();
    void flush();

    // Internal methods for callbacks
    void onDataReceived(uint8_t* data, uint32_t length);
    void onConnect();
    void onDisconnect();

private:
    uint16_t getRxBufferAvailable();
};
