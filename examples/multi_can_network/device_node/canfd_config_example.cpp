/**
 * @file canfd_config_example.cpp
 * @brief Example showing how to configure CAN FD at 5 Mbps data rate
 *
 * This shows the configuration needed for CAN FD with:
 * - Nominal bitrate: 1 Mbps (arbitration phase)
 * - Data bitrate: 5 Mbps (data phase with BRS)
 * - STM32G0 with 80 MHz FDCAN kernel clock
 */

#include "lumos.h"
#include "lumos_micro_brain.h"
#include "stm32g0xx_hal_fdcan.h"
#include "sys.h"
#include "gpio.h"

GPIO status_led(GPIOD, GPIO_PIN_2);

void setup(void)
{
    status_led.mode(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);

    // ===== CAN FD Configuration =====
    // Assuming 80 MHz FDCAN kernel clock on STM32G0

    // Method 1: Configure bit timing manually for precise control
    // Nominal bitrate: 1 Mbps (arbitration phase)
    // Calculation: 80 MHz / (5 * (1 + 13 + 2)) = 1 Mbps
    // - Prescaler: 5
    // - Seg1: 13 time quanta
    // - Seg2: 2 time quanta
    // - Total: 16 time quanta per bit
    
    // Data bitrate: 5 Mbps (data phase)
    // Calculation: 80 MHz / (2 * (1 + 5 + 2)) = 5 Mbps
    // - Prescaler: 2
    // - Seg1: 5 time quanta
    // - Seg2: 2 time quanta
    // - Total: 8 time quanta per bit

    CAN1.begin(1000000);  // Start with 1 Mbps nominal (sets base configuration)
    
    CAN1.enableFD(true)  // Enable CAN FD with Bit Rate Switching (BRS)
        .setNominalBitrate(5, 13, 2)   // 1 Mbps nominal for arbitration
        .setDataBitrate(2, 5, 2);      // 5 Mbps data rate for data phase

    // Alternative: Different nominal/data rates
    // For 500 kbps nominal, 5 Mbps data:
    // CAN1.begin(500000);
    // CAN1.enableFD(true)
    //     .setNominalBitrate(10, 13, 2)  // 500 kbps nominal
    //     .setDataBitrate(2, 5, 2);      // 5 Mbps data

    CAN1.setAcceptAll();  // Accept all messages
}

void loop(void)
{
    // Send CAN FD frame with 5 Mbps data phase
    uint8_t data[64];  // CAN FD supports up to 64 bytes
    for (int i = 0; i < 64; i++) {
        data[i] = i;
    }

    // Send 64-byte CAN FD frame
    // Data phase will run at 5 Mbps due to BRS (Bit Rate Switching)
    if (CAN1.send(0x100, data, 64, false)) {
        status_led.toggle();
    }

    DelayMs(100);

    // Receive CAN FD frames
    if (CAN1.available()) {
        uint32_t id;
        uint8_t rx_data[64];  // Buffer for up to 64 bytes
        uint8_t len;
        bool ext;

        if (CAN1.read(id, rx_data, len, ext)) {
            // Process received CAN FD frame
            // len can be 0-64 bytes for CAN FD frames
        }
    }
}

/**
 * Timing Parameter Reference (80 MHz FDCAN clock):
 *
 * Nominal Bitrates (Arbitration Phase):
 * - 125 kbps: prescaler=40, seg1=13, seg2=2
 * - 250 kbps: prescaler=20, seg1=13, seg2=2
 * - 500 kbps: prescaler=10, seg1=13, seg2=2
 * - 1 Mbps:   prescaler=5,  seg1=13, seg2=2
 *
 * Data Bitrates (Data Phase with BRS):
 * - 2 Mbps:   prescaler=5,  seg1=5,  seg2=2
 * - 4 Mbps:   prescaler=2,  seg1=8,  seg2=2 (or prescaler=4, seg1=3, seg2=2)
 * - 5 Mbps:   prescaler=2,  seg1=5,  seg2=2
 * - 8 Mbps:   prescaler=2,  seg1=3,  seg2=2
 *
 * Formula:
 * Bitrate = FDCAN_Clock / (Prescaler × (1 + Seg1 + Seg2))
 * where the "1" represents the Sync_Seg (always 1 time quantum)
 *
 * Sample Point:
 * Sample Point % = (1 + Seg1) / (1 + Seg1 + Seg2) × 100%
 * Example: (1 + 13) / (1 + 13 + 2) = 87.5% (good for most applications)
 *
 * Notes:
 * - CAN FD frames use nominal bitrate during arbitration (ID, control)
 * - When BRS is enabled, data phase uses higher data bitrate
 * - Both CAN nodes must use same nominal and data bitrates
 * - Bus length affects maximum reliable bitrate
 * - Shorter cables allow higher bitrates (5 Mbps: < 5 meters typical)
 */
