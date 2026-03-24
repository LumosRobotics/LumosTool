/**
 * @file main.cpp
 * @brief CAN Network Device Node
 *
 * This example demonstrates:
 * - Initializing FDCAN peripheral on STM32G0
 * - Sending periodic counter messages (50ms period)
 * - Receiving messages and echoing them back
 *
 * CAN Configuration:
 * - Bitrate: 500 kbps (standard CAN)
 * - TX ID: 0x100 (counter messages)
 * - RX ID: 0x200 (echo request)
 * - TX Echo ID: 0x201 (echo response)
 *
 * Hardware:
 * - CAN transceiver required (e.g., TJA1050, MCP2551)
 * - CAN_TX: PB11
 * - CAN_RX: PB12
 * - Status LED: PD2
 *
 * Network Protocol:
 * - Device sends counter (4 bytes, little-endian) on 0x100 every 50ms
 * - Device listens on 0x200, echoes received data back on 0x201
 */

#include "lumos.h"
#include "lumos_micro_brain.h"
#include "stm32g0xx_hal_fdcan.h"  // Force FDCAN HAL module detection
#include "sys.h"
#include "gpio.h"

// Status LED for visual feedback
GPIO status_led(GPIOD, GPIO_PIN_2);

// Timing variables
uint64_t last_send_time = 0;
const uint32_t SEND_PERIOD_MS = 50;

// Counter for periodic messages
uint32_t message_counter = 0;

// CAN message IDs
const uint32_t TX_COUNTER_ID = 0x100;  // Periodic counter messages
const uint32_t RX_ECHO_ID = 0x200;     // Echo request ID
const uint32_t TX_ECHO_ID = 0x201;     // Echo response ID

// Statistics
uint32_t messages_sent = 0;
uint32_t messages_received = 0;
uint32_t messages_echoed = 0;

/**
 * @brief Setup function - called once at startup
 */
void setup(void)
{
    // Initialize timing system
    InitMicrosecondTiming();

    // Configure status LED
    status_led.mode(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    status_led.low();

    // Flash LED to indicate startup
    for (int i = 0; i < 3; i++) {
        status_led.high();
        DelayMs(100);
        status_led.low();
        DelayMs(100);
    }

    // ===== Initialize CAN Bus =====
    // CAN1 is pre-configured in lumos_micro_brain.cpp
    // Pins: PB11 (TX), PB12 (RX), AF3_FDCAN1

    // Configuration Option 1: Classic CAN at 500 kbps (default)
    // CAN1.begin(500000);  // 500 kbps standard CAN

    // Configuration Option 2: CAN FD with 5 Mbps data rate
    // Uncomment the following to enable CAN FD mode:
    CAN1.begin(1000000);  // 1 Mbps nominal bitrate for arbitration
    CAN1.enableFD(true)   // Enable CAN FD with Bit Rate Switching
        .setNominalBitrate(5, 13, 2)   // 1 Mbps nominal (80MHz / (5 * 16))
        .setDataBitrate(2, 5, 2);      // 5 Mbps data rate (80MHz / (2 * 8))

    // Configure receive filter to accept messages with ID 0x200
    CAN1.setFilter(RX_ECHO_ID, 0x7FF, false);  // Standard ID, exact match

    // Alternatively, to accept all messages:
    // CAN1.setAcceptAll();

    // Record start time for periodic transmission
    last_send_time = GetCurrentTimeMs();

    // Flash LED rapidly to indicate CAN initialization complete
    for (int i = 0; i < 5; i++) {
        status_led.toggle();
        DelayMs(50);
    }
    status_led.low();
}

/**
 * @brief Loop function - called repeatedly
 */
void loop(void)
{
    uint64_t current_time = GetCurrentTimeMs();

    // ===== Periodic Counter Transmission (50ms) =====
    if (current_time - last_send_time >= SEND_PERIOD_MS) {
        last_send_time = current_time;

        // Prepare counter message (4 bytes, little-endian)
        uint8_t data[4];
        data[0] = (message_counter >> 0) & 0xFF;
        data[1] = (message_counter >> 8) & 0xFF;
        data[2] = (message_counter >> 16) & 0xFF;
        data[3] = (message_counter >> 24) & 0xFF;

        // Send counter message
        if (CAN1.send(TX_COUNTER_ID, data, 4, false)) {
            messages_sent++;
            message_counter++;
            
            // Brief LED pulse on successful send
            status_led.high();
        }
    }

    // ===== Receive and Echo Messages =====
    if (CAN1.available()) {
        uint32_t rx_id;
        uint8_t rx_data[8];
        uint8_t rx_length;
        bool rx_extended;

        // Read received message
        if (CAN1.read(rx_id, rx_data, rx_length, rx_extended)) {
            messages_received++;

            // Check if this is an echo request
            if (rx_id == RX_ECHO_ID) {
                // Echo the message back on TX_ECHO_ID
                if (CAN1.send(TX_ECHO_ID, rx_data, rx_length, rx_extended)) {
                    messages_echoed++;
                    
                    // Toggle LED to indicate echo
                    status_led.toggle();
                    DelayMs(1);  // Brief visual indicator
                }
            }
        }
    }

    // Turn off LED after send pulse
    if (status_led.read()) {
        DelayMs(1);
        status_led.low();
    }

    // Small delay to prevent tight loop
    DelayMs(1);
}

/**
 * Network Protocol Details:
 *
 * Counter Message (TX, ID 0x100):
 * - Sent every 50ms
 * - Format: 4 bytes, little-endian uint32_t
 * - [Byte 0] Counter LSB
 * - [Byte 1] Counter byte 1
 * - [Byte 2] Counter byte 2
 * - [Byte 3] Counter MSB
 *
 * Echo Request (RX, ID 0x200):
 * - Accepted from any device on the bus
 * - Data format: arbitrary (1-8 bytes)
 * - Device echoes received data back unchanged
 *
 * Echo Response (TX, ID 0x201):
 * - Same data as received echo request
 * - Same data length as received
 * - Sent immediately after receiving echo request
 *
 * Example Usage:
 * 1. Connect two or more LumosMicroBrain boards via CAN bus
 * 2. All devices send counter messages on 0x100
 * 3. Send message on 0x200 to any device, it will echo on 0x201
 * 4. Use a CAN analyzer to observe the traffic
 *
 * Troubleshooting:
 * - LED not flashing: CAN initialization failed or no CAN bus activity
 * - Messages not received: Check CAN termination resistors (120Ω)
 * - Bus errors: Verify TX/RX pin connections and transceiver power
 * - No echo: Verify filter configuration and RX ID
 */
