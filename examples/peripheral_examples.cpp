/**
 * Peripheral Usage Examples for LumosBrain Board
 *
 * This file contains example usage of the Serial, CAN, and I2C abstractions
 * defined in jst_shield.h
 */

#include "jst_shield.h"

// ============================================================================
// Serial Communication Examples
// ============================================================================

void serial_example()
{
    // Initialize Serial1 (UART7) at 115200 baud
    Serial1.begin(115200);

    // Configure with fluent API
    Serial2.begin(9600)
           .setParity(UART_PARITY_EVEN);

    // Note: Actual read/write methods would be implemented based on your needs
}

// ============================================================================
// CAN Communication Examples
// ============================================================================

void can_example()
{
    // Basic CAN initialization at 500 kbps
    CAN1.begin(500000);

    // Advanced configuration with fluent API
    CAN2.begin(1000000)
        .setMode(FDCAN_MODE_NORMAL);

    // Send a standard CAN message
    uint8_t data[] = {0x11, 0x22, 0x33, 0x44};
    CAN1.send(0x123, data, 4);

    // Send an extended ID message
    CAN1.send(0x12345678, data, 4, true);

    // Send a remote frame
    CAN1.sendRemote(0x456);

    // Receive messages
    if (CAN1.available()) {
        uint32_t id;
        uint8_t rxData[8];
        uint8_t length;
        bool extended;

        if (CAN1.read(id, rxData, length, extended)) {
            // Process received message
            if (extended) {
                // Handle extended ID message
            }
        }
    }

    // Configure message filtering
    CAN1.setFilter(0x100, 0x700, false);  // Accept IDs 0x100-0x1FF

    // Accept all messages (default)
    CAN1.setAcceptAll();

    // Check for bus errors
    uint32_t errors = CAN1.getErrorCount();
    if (CAN1.isBusOff()) {
        // Handle bus-off condition
        CAN1.end();
        CAN1.begin(500000);  // Restart
    }
}

// ============================================================================
// I2C Communication Examples
// ============================================================================

void i2c_example()
{
    // Initialize i2c1 at default 100 kHz (Standard Mode)
    i2c1.begin();

    // Initialize i2c2 at 400 kHz (Fast Mode)
    i2c2.begin(400000);

    // Advanced configuration with fluent API
    i2c4.begin(100000)
        .setClock(400000);

    // Basic write operation
    uint8_t txData[] = {0x10, 0x20, 0x30};
    i2c1.write(0x50, txData, 3);  // Write to device at address 0x50

    // Basic read operation
    uint8_t rxData[4];
    i2c1.read(0x50, rxData, 4);  // Read from device at address 0x50

    // Register read/write (common for I2C sensors)
    i2c1.writeRegister(0x68, 0x6B, 0x00);  // Wake up MPU6050 (addr=0x68, reg=0x6B)

    uint8_t whoami;
    i2c1.readRegister(0x68, 0x75, whoami);  // Read WHO_AM_I register

    // 16-bit register operations
    uint16_t value16 = 0x1234;
    i2c1.writeRegister16(0x40, 0x10, value16);  // Write 16-bit value

    uint16_t result;
    i2c1.readRegister16(0x40, 0x10, result);  // Read 16-bit value

    // Read multiple registers (burst read)
    uint8_t accelData[6];
    i2c1.readRegisters(0x68, 0x3B, accelData, 6);  // Read accelerometer data

    // Device detection and scanning
    if (i2c1.probe(0x68)) {
        // Device at address 0x68 is present
    }

    // Scan the I2C bus for all devices
    uint8_t devices[128];
    uint8_t count;
    i2c1.scan(devices, count);

    // Print found devices
    for (uint8_t i = 0; i < count; i++) {
        // devices[i] contains the 7-bit address
    }

    // Error checking
    if (!i2c1.isReady()) {
        uint32_t error = i2c1.getError();
        // Handle error
    }
}

// ============================================================================
// Practical Application Examples
// ============================================================================

void read_mpu6050_sensor()
{
    // Initialize I2C
    i2c1.begin(400000);  // Fast mode for sensor

    // Check if MPU6050 is present
    if (!i2c1.probe(0x68)) {
        // Sensor not found
        return;
    }

    // Wake up the sensor
    i2c1.writeRegister(0x68, 0x6B, 0x00);

    // Read accelerometer and gyroscope data
    uint8_t rawData[14];
    i2c1.readRegisters(0x68, 0x3B, rawData, 14);

    // Parse the data
    int16_t accelX = (rawData[0] << 8) | rawData[1];
    int16_t accelY = (rawData[2] << 8) | rawData[3];
    int16_t accelZ = (rawData[4] << 8) | rawData[5];
    int16_t temp = (rawData[6] << 8) | rawData[7];
    int16_t gyroX = (rawData[8] << 8) | rawData[9];
    int16_t gyroY = (rawData[10] << 8) | rawData[11];
    int16_t gyroZ = (rawData[12] << 8) | rawData[13];

    // Process sensor data...
}

void can_bridge_example()
{
    // Initialize two CAN buses
    CAN1.begin(500000);
    CAN2.begin(250000);

    // Bridge messages from CAN1 to CAN2
    while (1) {
        if (CAN1.available()) {
            uint32_t id;
            uint8_t data[8];
            uint8_t length;
            bool extended;

            if (CAN1.read(id, data, length, extended)) {
                // Forward message to CAN2
                CAN2.send(id, data, length, extended);
            }
        }

        if (CAN2.available()) {
            uint32_t id;
            uint8_t data[8];
            uint8_t length;
            bool extended;

            if (CAN2.read(id, data, length, extended)) {
                // Forward message to CAN1
                CAN1.send(id, data, length, extended);
            }
        }
    }
}

void multi_sensor_i2c()
{
    // Multiple I2C buses for different sensors
    i2c1.begin(400000);  // IMU on i2c1
    i2c2.begin(100000);  // Environmental sensor on i2c2
    i2c4.begin(400000);  // Display on i2c4

    // Read from different sensors concurrently
    uint8_t imuData[6];
    i2c1.readRegisters(0x68, 0x3B, imuData, 6);

    uint8_t tempData[2];
    i2c2.readRegisters(0x40, 0x00, tempData, 2);

    uint8_t displayCmd[] = {0x00, 0x01};
    i2c4.write(0x3C, displayCmd, 2);
}
