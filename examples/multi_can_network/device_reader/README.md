# CAN Network Reader Node

This example demonstrates a CAN network reader that communicates with the `device_node` example. It sends echo requests, receives counter and echo response messages, and outputs all data to USB serial.

## Features

- **Echo Request Transmission**: Sends incrementing counter on CAN ID `0x200` every 100ms
- **Counter Message Reception**: Reads counter messages from device_node on ID `0x100` (50ms period)
- **Echo Response Reception**: Reads echo responses from device_node on ID `0x201`
- **USB Serial Output**: Displays all received messages in human-readable format

## Hardware Requirements

### CAN Transceiver
Same as device_node - requires a CAN transceiver IC:
- **TJA1050** (NXP) - 5V transceiver
- **MCP2551** (Microchip) - 5V transceiver
- **SN65HVD230** (Texas Instruments) - 3.3V transceiver

### Pin Connections

| STM32G0 Pin | Function | Connection |
|-------------|----------|------------|
| PB11        | CAN_TX   | Transceiver TXD |
| PB12        | CAN_RX   | Transceiver RXD |
| PA11        | USB D-   | USB connector |
| PA12        | USB D+   | USB connector |
| PD2         | Status LED | LED (optional) |

### USB Connection
- Connect the LumosMicroBrain to your PC via USB
- The board will enumerate as a virtual COM port
- Use any serial terminal at 115200 baud

## Network Protocol

### Message Flow

```
device_reader                    device_node
     |                                |
     |------ Echo Request (0x200) --->|  (100ms period)
     |                                |
     |<----- Echo Response (0x201) ---|  (immediate)
     |                                |
     |<----- Counter (0x100) ----------|  (50ms period)
```

### Message IDs

| ID    | Direction | Source | Description | Data Format | Period |
|-------|-----------|--------|-------------|-------------|--------|
| 0x200 | TX        | reader | Echo Request | 4 bytes (uint32_t LE) | 100ms |
| 0x201 | RX        | node   | Echo Response | 4 bytes (echoed data) | On-demand |
| 0x100 | RX        | node   | Counter | 4 bytes (uint32_t LE) | 50ms |

### Echo Request Format (ID 0x200)
```
Byte 0: Counter LSB
Byte 1: Counter byte 1
Byte 2: Counter byte 2
Byte 3: Counter MSB
```

Counter increments with each transmission: 0, 1, 2, 3, ...

## Serial Output Format

### Counter Messages
```
Counter: 1234 (0x4d2)
Counter: 1235 (0x4d3)
Counter: 1236 (0x4d4)
```

### Echo Responses
```
Echo Response: 5678 (0x162e) - Data: 2e 16 00 00
Echo Response: 5679 (0x162f) - Data: 2f 16 00 00
```

## Building

```bash
cd /Users/danielpi/work/LumosTool/examples/multi_can_network/device_reader
lumos_dev build
```

## Testing

### Basic Test (Two Boards)
1. Flash `device_node` firmware to one LumosMicroBrain board
2. Flash `device_reader` firmware to another LumosMicroBrain board
3. Connect both boards to the same CAN bus
4. Add 120Ω termination resistors at both ends
5. Connect device_reader board to PC via USB
6. Open serial terminal (e.g., Arduino IDE Serial Monitor, PuTTY, screen)
7. Set baud rate to 115200
8. Observe counter and echo response messages

### Expected Output
```
=== CAN Network Reader Node ===
Initializing...
CAN initialized at 500 kbps
Listening on IDs: 0x100 (counter), 0x201 (echo response)
Sending echo requests on ID: 0x200

Counter: 0 (0x0)
Counter: 1 (0x1)
Echo Response: 0 (0x0) - Data: 00 00 00 00
Counter: 2 (0x2)
Counter: 3 (0x3)
Echo Response: 1 (0x1) - Data: 01 00 00 00
Counter: 4 (0x4)
...
```

### Serial Terminal Commands

**Linux/Mac:**
```bash
screen /dev/ttyACM0 115200
# or
picocom /dev/ttyACM0 -b 115200
```

**Windows:**
- Use PuTTY or Arduino IDE Serial Monitor
- Select the appropriate COM port
- Set baud rate to 115200

## Configuration

### CAN Bitrate
Default: **500 kbps** (must match device_node)

To change bitrate, modify in `main.cpp`:
```cpp
CAN1.begin(500000);  // Change to desired bitrate
```

### CAN FD Mode
To enable CAN FD at 5 Mbps data rate (must match device_node):
```cpp
// Comment out Classic CAN configuration
// CAN1.begin(500000);

// Uncomment CAN FD configuration
CAN1.begin(1000000);  // 1 Mbps nominal
CAN1.enableFD(true)
    .setNominalBitrate(5, 13, 2)   // 1 Mbps nominal
    .setDataBitrate(2, 5, 2);      // 5 Mbps data
```

### Echo Request Period
To change the echo request transmission period:
```cpp
const uint32_t ECHO_REQUEST_PERIOD_MS = 100;  // Change to desired period
```

## Status LED Behavior

- **3 slow blinks**: Startup
- **5 fast blinks**: Initialization complete
- **Brief pulse**: Echo request sent
- **Toggle**: Message received (counter or echo response)

## Troubleshooting

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| No serial output | USB not connected | Verify USB cable and driver installation |
| Blank terminal | Wrong baud rate | Set terminal to 115200 baud |
| No counter messages | device_node not running | Check device_node board power and CAN connection |
| No echo responses | CAN bus issue | Verify termination resistors and wiring |
| Garbled data | Bitrate mismatch | Ensure both devices use same CAN bitrate |
| USB not enumerated | Driver issue | Install STM32 VCP drivers (Windows) |

## Statistics Variables

The code tracks the following statistics (accessible via debugger):
- `echo_requests_sent` - Total echo requests transmitted
- `counter_messages_received` - Total counter messages received
- `echo_responses_received` - Total echo responses received
- `last_received_counter` - Most recent counter value
- `echo_request_counter` - Current echo request counter value

## Extending the Example

### Adding Timestamp to Serial Output
```cpp
SerialPgm.print("[");
SerialPgm.print(GetCurrentTimeMs());
SerialPgm.print(" ms] Counter: ");
SerialPgm.println(counter_value);
```

### Processing Additional Message IDs
```cpp
else if (rx_id == 0x300) {
    // Handle custom message
    SerialPgm.println("Custom message received");
}
```

### Binary Serial Output
```cpp
// Send raw binary data instead of text
SerialPgm.write(rx_data, rx_length);
```

## References

- [STM32G0 USB CDC Guide](https://www.st.com/resource/en/application_note/an4879-usb-hardware-and-pcb-guidelines-using-stm32-mcus-stmicroelectronics.pdf)
- [CAN Protocol Tutorial](https://www.kvaser.com/can-protocol-tutorial/)
- See `device_node` README for CAN network details
