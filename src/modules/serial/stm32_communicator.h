#pragma once

#include "serial.h"
#include <string>
#include <vector>
#include <cstdint>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

namespace SimpleSerial {

/**
 * @brief Firmware data structure for flashing
 */
struct FirmwareData {
    uint32_t start_address;
    std::vector<uint8_t> data;
};

/**
 * @brief STM32 bootloader response codes
 */
enum class BootloaderResponse : uint8_t {
    ACK = 0x79,
    NACK = 0x1F
};

/**
 * @brief STM32 bootloader commands
 */
enum class BootloaderCommand : uint8_t {
    GET = 0x00,
    GET_VERSION = 0x01,
    GET_ID = 0x02,
    READ_MEMORY = 0x11,
    GO = 0x21,
    WRITE_MEMORY = 0x31,
    ERASE = 0x43,
    EXTENDED_ERASE = 0x44,
    WRITE_PROTECT = 0x63,
    WRITE_UNPROTECT = 0x73,
    READOUT_PROTECT = 0x82,
    READOUT_UNPROTECT = 0x92
};

/**
 * @brief STM32 communication and flashing class
 *
 * Handles communication with STM32 microcontrollers for both flashing
 * firmware and runtime serial communication. The class manages a serial
 * connection that can be switched between different ports at runtime.
 */
class STM32Communicator {
public:
    /**
     * @brief Callback type for received data
     * @param data Pointer to received data
     * @param length Number of bytes received
     */
    using DataCallback = std::function<void(const uint8_t* data, size_t length)>;

    STM32Communicator();
    ~STM32Communicator();

    /**
     * @brief Connect to a serial port
     * @param port_name Name of the serial port
     * @param baud_rate Baud rate for communication (default: 115200)
     * @return true if successful, false otherwise
     */
    bool Connect(const std::string& port_name, int baud_rate = 115200);

    /**
     * @brief Disconnect from the current serial port
     */
    void Disconnect();

    /**
     * @brief Check if connected to a serial port
     * @return true if connected, false otherwise
     */
    bool IsConnected() const;

    /**
     * @brief Get the current port name
     * @return Port name, or empty string if not connected
     */
    std::string GetPortName() const;

    /**
     * @brief Enter STM32 bootloader mode
     * @param pulse_dtr If true, pulse DTR to reset MCU (default: true)
     * @return true if bootloader responded, false otherwise
     */
    bool EnterBootloader(bool pulse_dtr = true);

    /**
     * @brief Flash firmware to the MCU
     * @param firmware Firmware data to flash
     * @param erase_all If true, perform full chip erase before flashing
     * @return true if successful, false otherwise
     */
    bool Flash(const FirmwareData& firmware, bool erase_all = true);

    /**
     * @brief Start monitoring serial data from MCU
     * Starts a background thread that receives and processes data
     * @param callback Function to call when data is received (optional)
     * @return true if started successfully, false otherwise
     */
    bool StartMonitoring(DataCallback callback = nullptr);

    /**
     * @brief Stop monitoring serial data
     */
    void StopMonitoring();

    /**
     * @brief Check if monitoring is active
     * @return true if monitoring, false otherwise
     */
    bool IsMonitoring() const;

    /**
     * @brief Send data to the MCU
     * @param data Pointer to data to send
     * @param length Number of bytes to send
     * @return Number of bytes sent, -1 on error
     */
    int Send(const uint8_t* data, size_t length);

    /**
     * @brief Send a string to the MCU
     * @param str String to send
     * @return Number of bytes sent, -1 on error
     */
    int Send(const std::string& str);

    /**
     * @brief Send a vector of bytes to the MCU
     * @param data Vector of bytes to send
     * @return Number of bytes sent, -1 on error
     */
    int Send(const std::vector<uint8_t>& data);

    /**
     * @brief Get the last error message
     * @return Error message string
     */
    std::string GetLastError() const;

private:
    Serial serial_;
    std::string port_name_;
    int baud_rate_;
    bool is_connected_;
    std::string last_error_;

    // Monitoring thread
    std::thread monitor_thread_;
    std::atomic<bool> monitoring_active_;
    std::mutex serial_mutex_;
    DataCallback data_callback_;

    // Private helper methods
    void SetError(const std::string& error);
    void MonitorThreadFunc();

    // Bootloader protocol helpers
    bool SendCommand(BootloaderCommand cmd);
    bool SendCommandWithAddress(BootloaderCommand cmd, uint32_t address);
    bool WaitForAck(int timeout_ms = 1000);
    bool WriteMemory(uint32_t address, const uint8_t* data, size_t length);
    bool EraseMemory(bool full_erase = true);
    uint8_t CalculateChecksum(const uint8_t* data, size_t length);
};

} // namespace SimpleSerial
