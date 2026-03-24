# CAN Network Device Node

This example demonstrates a complete CAN network node implementation on the LumosMicroBrain (STM32G0B1CBU3).

## Features

- **Periodic Counter Messages**: Sends a 32-bit counter on CAN ID `0x100` every 50ms
- **Echo Service**: Listens on CAN ID `0x200` and echoes received messages back on `0x201`
- **Visual Feedback**: Status LED (PD2) provides visual indication of CAN activity

## Hardware Requirements

### CAN Transceiver
A CAN transceiver IC is required to interface with the physical CAN bus. Common options:
- **TJA1050** (NXP) - 5V transceiver
- **MCP2551** (Microchip) - 5V transceiver
- **SN65HVD230** (Texas Instruments) - 3.3V transceiver

### Pin Connections

| STM32G0 Pin | Function | Transceiver Pin |
|-------------|----------|----------------|
| PB11        | CAN_TX   | TXD            |
| PB12        | CAN_RX   | RXD            |
| PD2         | Status LED | -             |

### CAN Bus Termination
- 120Ω termination resistors required at both ends of the CAN bus
- Connect between CAN_H and CAN_L

## CAN Network Protocol

### Message IDs

| ID    | Direction | Description | Data Format | Period |
|-------|-----------|-------------|-------------|--------|
| 0x100 | TX        | Counter     | 4 bytes (uint32_t, little-endian) | 50ms |
| 0x200 | RX        | Echo Request | 1-8 bytes (arbitrary) | On-demand |
| 0x201 | TX        | Echo Response | Same as received | Immediate |

### Counter Message Format (ID 0x100)
```
Byte 0: Counter LSB
Byte 1: Counter byte 1
Byte 2: Counter byte 2
Byte 3: Counter MSB
```

Counter increments with each transmission: 0, 1, 2, 3, ... (wraps at 2^32)

### Echo Service (ID 0x200 → 0x201)
Any device can send a message on ID `0x200` with arbitrary data (1-8 bytes). The node will immediately echo the same data back on ID `0x201`.

## Configuration

### CAN Bitrate
Default: **500 kbps** (standard CAN)

To change bitrate, modify in `main.cpp`:
```cpp
CAN1.begin(500000);  // Change to desired bitrate
```

Common bitrates:
- 125 kbps (long distance, noisy environments)
- 250 kbps
- 500 kbps (default)
- 1 Mbps (short distance, low noise)

### Message IDs
To customize message IDs, modify in `main.cpp`:
```cpp
const uint32_t TX_COUNTER_ID = 0x100;  // Periodic counter
const uint32_t RX_ECHO_ID = 0x200;     // Echo request
const uint32_t TX_ECHO_ID = 0x201;     // Echo response
```

### Transmission Period
To change the counter transmission period, modify:
```cpp
const uint32_t SEND_PERIOD_MS = 50;  // Change to desired period
```

## Building

```bash
cd /Users/danielpi/work/LumosTool/examples/multi_can_network/device_node
lumos_dev build
```

## Flashing

Flash the firmware to your LumosMicroBrain board using your preferred method (ST-Link, DFU, etc.)

## Testing

### Using CAN Analyzer
1. Connect a CAN analyzer (e.g., PEAK PCAN-USB, CANable)
2. Set to 500 kbps
3. Observe periodic messages on ID 0x100
4. Send message on ID 0x200, observe echo on 0x201

### Multi-Device Network
1. Flash multiple LumosMicroBrain boards with this firmware
2. Connect all devices to same CAN bus
3. All devices will send counters on 0x100
4. Any device can be used as echo responder

### Example CAN Frames

**Counter messages (every 50ms):**
```
ID: 0x100  Data: 00 00 00 00  (counter = 0)
ID: 0x100  Data: 01 00 00 00  (counter = 1)
ID: 0x100  Data: 02 00 00 00  (counter = 2)
```

**Echo test:**
```
TX: ID: 0x200  Data: 11 22 33 44
RX: ID: 0x201  Data: 11 22 33 44
```

## Status LED Behavior

- **3 slow blinks**: Startup
- **5 fast blinks**: CAN initialization complete
- **Brief pulse (50ms)**: Counter message sent
- **Toggle**: Echo message received and sent

## Troubleshooting

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| LED doesn't blink on startup | Power issue | Check 3.3V supply |
| No CAN messages | Transceiver not powered | Verify transceiver power |
| Bus errors | Wrong bitrate | Match all devices to same bitrate |
| No message reception | Missing termination | Add 120Ω resistors |
| Intermittent errors | Loose connections | Check TX/RX wiring |
| High error count | Bus noise | Add ferrite beads, shorten cables |

## Statistics Variables

The code tracks the following statistics (accessible via debugger):
- `messages_sent` - Total counter messages transmitted
- `messages_received` - Total messages received
- `messages_echoed` - Total echo responses sent
- `message_counter` - Current counter value

## Extending the Example

### Adding More Message Types
```cpp
const uint32_t TX_SENSOR_ID = 0x300;
// In loop():
if (current_time - last_sensor_time >= 100) {
    uint8_t sensor_data[2] = {temperature, humidity};
    CAN1.send(TX_SENSOR_ID, sensor_data, 2, false);
}
```

### Accepting Multiple IDs
```cpp
// Accept range of IDs (0x200-0x20F)
CAN1.setFilter(0x200, 0x7F0, false);
```

### Extended IDs (29-bit)
```cpp
CAN1.send(0x12345678, data, len, true);  // true = extended ID
```

## References

- [STM32G0 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0444-stm32g0x0-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [FDCAN Application Note](https://www.st.com/resource/en/application_note/an5348-fdcan-protocol-used-in-the-stm32-mcus-stmicroelectronics.pdf)
- [CAN Specification](https://www.kvaser.com/can-protocol-tutorial/)
