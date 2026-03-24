/**
 * @file main.cpp
 * @brief CAN Network Reader Node
 *
 * This example demonstrates:
 * - Sending periodic echo request messages on CAN ID 0x200
 * - Reading counter messages from device_node on ID 0x100
 * - Reading echo response messages on ID 0x201
 * - Outputting all received CAN data to USB serial (SerialPgm)
 *
 * Network Protocol:
 * - TX: Echo requests on 0x200 every 100ms (triggers device_node echo)
 * - RX: Counter messages on 0x100 from device_node (50ms period)
 * - RX: Echo responses on 0x201 from device_node (echoed data)
 * - Serial output: All received messages formatted as human-readable text
 *
 * Hardware:
 * - CAN transceiver required (e.g., TJA1050, MCP2551)
 * - CAN_TX: PB11
 * - CAN_RX: PB12
 * - USB: PA11 (D-), PA12 (D+)
 * - Status LED: PD2
 */

#include "lumos.h"
#include "lumos_micro_brain.h"
#include "stm32g0xx_hal_fdcan.h"  // Force FDCAN HAL module detection
#include "stm32g0xx_hal_uart.h"   // Force UART HAL module detection
#include "sys.h"
#include "gpio.h"

// Status LED for visual feedback
GPIO status_led(GPIOD, GPIO_PIN_2);

// Timing variables
uint64_t last_echo_request_time = 0;
const uint32_t ECHO_REQUEST_PERIOD_MS = 100;

// Echo request counter
uint32_t echo_request_counter = 0;

// CAN message IDs (must match device_node)
const uint32_t RX_COUNTER_ID = 0x100;     // Counter messages from device_node
const uint32_t TX_ECHO_REQUEST_ID = 0x200; // Echo requests to device_node
const uint32_t RX_ECHO_RESPONSE_ID = 0x201; // Echo responses from device_node

// Statistics
uint32_t echo_requests_sent = 0;
uint32_t counter_messages_received = 0;
uint32_t echo_responses_received = 0;
uint32_t last_received_counter = 0;

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

    // ===== Initialize USB Serial =====
    SerialPgm.begin(115200);  // USB virtual COM port
    DelayMs(500);  // Wait for USB enumeration

    SerialPgm.println("=== CAN Network Reader Node ===");
    SerialPgm.println("Initializing...");

    // ===== Initialize CAN Bus =====
    // Configuration Option 1: Classic CAN at 500 kbps (default, matches device_node)
    CAN1.begin(500000);  // 500 kbps standard CAN

    // Configuration Option 2: CAN FD with 5 Mbps data rate
    // Uncomment to match device_node if it's configured for CAN FD:
    /*
    CAN1.begin(1000000);  // 1 Mbps nominal bitrate for arbitration
    CAN1.enableFD(true)   // Enable CAN FD with Bit Rate Switching
        .setNominalBitrate(5, 13, 2)   // 1 Mbps nominal (80MHz / (5 * 16))
        .setDataBitrate(2, 5, 2);      // 5 Mbps data rate (80MHz / (2 * 8))
    */

    // Configure receive filter to accept messages from device_node
    // Accept both counter (0x100) and echo response (0x201)
    CAN1.setAcceptAll();  // Accept all messages for simplicity

    SerialPgm.println("CAN initialized at 500 kbps");
    SerialPgm.println("Listening on IDs: 0x100 (counter), 0x201 (echo response)");
    SerialPgm.println("Sending echo requests on ID: 0x200");
    SerialPgm.println();

    // Flash LED rapidly to indicate initialization complete
    for (int i = 0; i < 5; i++) {
        status_led.toggle();
        DelayMs(50);
    }
    status_led.low();

    // Record start time for periodic transmission
    last_echo_request_time = GetCurrentTimeMs();
}

/**
 * @brief Loop function - called repeatedly
 */
void loop(void)
{
    uint64_t current_time = GetCurrentTimeMs();

    // ===== Periodic Echo Request Transmission (100ms) =====
    if (current_time - last_echo_request_time >= ECHO_REQUEST_PERIOD_MS) {
        last_echo_request_time = current_time;

        // Prepare echo request message (4 bytes, little-endian counter)
        uint8_t data[4];
        data[0] = (echo_request_counter >> 0) & 0xFF;
        data[1] = (echo_request_counter >> 8) & 0xFF;
        data[2] = (echo_request_counter >> 16) & 0xFF;
        data[3] = (echo_request_counter >> 24) & 0xFF;

        // Send echo request message
        if (CAN1.send(TX_ECHO_REQUEST_ID, data, 4, false)) {
            echo_requests_sent++;
            echo_request_counter++;

            // Brief LED pulse on successful send
            status_led.high();
        }
    }

    // ===== Receive and Process CAN Messages =====
    if (CAN1.available()) {
        uint32_t rx_id;
        uint8_t rx_data[8];
        uint8_t rx_length;
        bool rx_extended;

        // Read received message
        if (CAN1.read(rx_id, rx_data, rx_length, rx_extended)) {

            // Process counter message from device_node (ID 0x100)
            if (rx_id == RX_COUNTER_ID) {
                counter_messages_received++;

                // Parse counter value (4 bytes, little-endian)
                if (rx_length >= 4) {
                    uint32_t counter_value = (uint32_t)rx_data[0] |
                                            ((uint32_t)rx_data[1] << 8) |
                                            ((uint32_t)rx_data[2] << 16) |
                                            ((uint32_t)rx_data[3] << 24);

                    last_received_counter = counter_value;

                    // Print to serial
                    SerialPgm.print("Counter: ");
                    SerialPgm.print(counter_value);
                    SerialPgm.print(" (0x");
                    SerialPgm.print(counter_value, 16);
                    SerialPgm.println(")");
                }

                status_led.toggle();
            }

            // Process echo response from device_node (ID 0x201)
            else if (rx_id == RX_ECHO_RESPONSE_ID) {
                echo_responses_received++;

                // Parse echoed counter value (4 bytes, little-endian)
                if (rx_length >= 4) {
                    uint32_t echoed_value = (uint32_t)rx_data[0] |
                                           ((uint32_t)rx_data[1] << 8) |
                                           ((uint32_t)rx_data[2] << 16) |
                                           ((uint32_t)rx_data[3] << 24);

                    // Print to serial
                    SerialPgm.print("Echo Response: ");
                    SerialPgm.print(echoed_value);
                    SerialPgm.print(" (0x");
                    SerialPgm.print(echoed_value, 16);
                    SerialPgm.print(") - Data: ");

                    // Print raw bytes
                    for (uint8_t i = 0; i < rx_length; i++) {
                        if (rx_data[i] < 0x10) SerialPgm.print("0");
                        SerialPgm.print(rx_data[i], 16);
                        if (i < rx_length - 1) SerialPgm.print(" ");
                    }
                    SerialPgm.println();
                }

                status_led.toggle();
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
 * Network Reader Protocol Details:
 *
 * Echo Request (TX, ID 0x200):
 * - Sent every 100ms
 * - Format: 4 bytes, little-endian uint32_t
 * - Contains incrementing counter value
 * - Triggers device_node to echo back on 0x201
 *
 * Counter Message (RX, ID 0x100):
 * - Received from device_node every 50ms
 * - Format: 4 bytes, little-endian uint32_t
 * - Counter increments: 0, 1, 2, 3, ...
 * - Displayed on USB serial
 *
 * Echo Response (RX, ID 0x201):
 * - Received from device_node after sending echo request
 * - Format: 4 bytes (same as echo request)
 * - Contains the counter value we sent
 * - Displayed on USB serial with raw bytes
 *
 * Serial Output Format:
 * - Counter messages: "Counter: 1234 (0x4d2)"
 * - Echo responses: "Echo Response: 5678 (0x162e) - Data: 2e 16 00 00"
 *
 * Example Usage:
 * 1. Flash device_node to one LumosMicroBrain board
 * 2. Flash device_reader to another LumosMicroBrain board
 * 3. Connect both boards to same CAN bus with proper termination
 * 4. Connect device_reader to PC via USB
 * 5. Open serial terminal at 115200 baud
 * 6. Observe counter messages and echo responses
 *
 * Troubleshooting:
 * - No serial output: Check USB connection and serial terminal settings
 * - No counter messages: Verify device_node is running and connected
 * - No echo responses: Check CAN bus termination and wiring
 * - Mismatched data: Ensure both devices use same CAN bitrate
 */
