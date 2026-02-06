#pragma once

// Platform-specific HAL headers
// Note: FDCAN is only available on certain STM32 families (G0, G4, H5, H7)
#if defined(STM32H7)
    #include "stm32h7xx_hal.h"
    #include "stm32h7xx_hal_fdcan.h"
#elif defined(STM32G0)
    #include "stm32g0xx_hal.h"
    #include "stm32g0xx_hal_fdcan.h"
#elif defined(STM32G4)
    #include "stm32g4xx_hal.h"
    #include "stm32g4xx_hal_fdcan.h"
#elif defined(STM32H5)
    #include "stm32h5xx_hal.h"
    #include "stm32h5xx_hal_fdcan.h"
#elif defined(STM32F4)
    // F4 uses classic CAN (bxCAN), not FDCAN
    #include "stm32f4xx_hal.h"
    #include "stm32f4xx_hal_can.h"
    #error "CAN wrapper currently supports FDCAN only. F4 uses bxCAN which has different API."
#else
    #error "Unsupported STM32 platform. Define STM32H7, STM32G0, STM32G4, or STM32H5."
#endif

// CAN (FDCAN) Class - Flexible Data-rate CAN
// Usage Example:
//   CAN1.begin(500000);  // Start CAN at 500 kbps
//   uint8_t data[] = {0x11, 0x22, 0x33};
//   CAN1.send(0x123, data, 3);  // Send message with ID 0x123
//
//   if (CAN1.available()) {
//       uint32_t id;
//       uint8_t data[8];
//       uint8_t len;
//       bool ext;
//       CAN1.read(id, data, len, ext);
//   }
class CAN
{
private:
    FDCAN_HandleTypeDef fdcan_handle_;

    // GPIO configuration
    GPIO_TypeDef* tx_port_;
    uint16_t tx_pin_;
    GPIO_TypeDef* rx_port_;
    uint16_t rx_pin_;
    uint32_t alternate_function_;

public:
    CAN() = delete;
    CAN(FDCAN_GlobalTypeDef* fdcan_instance,
        GPIO_TypeDef* tx_port, uint16_t tx_pin,
        GPIO_TypeDef* rx_port, uint16_t rx_pin,
        uint32_t alternate_function);

    void begin(const uint32_t bitrate = 500000);
    void end();

    // Fluent API setters
    CAN& setMode(const uint32_t mode)
    {
        fdcan_handle_.Init.Mode = mode;
        HAL_FDCAN_Init(&fdcan_handle_);
        return *this;
    }

    CAN& setNominalBitrate(const uint32_t prescaler, const uint32_t seg1, const uint32_t seg2)
    {
        fdcan_handle_.Init.NominalPrescaler = prescaler;
        fdcan_handle_.Init.NominalTimeSeg1 = seg1;
        fdcan_handle_.Init.NominalTimeSeg2 = seg2;
        HAL_FDCAN_Init(&fdcan_handle_);
        return *this;
    }

    // Message transmission
    bool send(uint32_t id, const uint8_t* data, uint8_t length, bool extended = false);
    bool sendRemote(uint32_t id, bool extended = false);

    // Message reception
    bool available();
    bool read(uint32_t& id, uint8_t* data, uint8_t& length, bool& extended);

    // Filter configuration
    void setFilter(uint32_t id, uint32_t mask, bool extended = false);
    void setAcceptAll();

    // Status
    uint32_t getErrorCount();
    bool isBusOff();
};
