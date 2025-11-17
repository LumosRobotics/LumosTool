#include "stm32_communicator.h"
#include <iostream>
#include <chrono>
#include <thread>

namespace SimpleSerial {

STM32Communicator::STM32Communicator()
    : baud_rate_(115200)
    , is_connected_(false)
    , monitoring_active_(false)
{
}

STM32Communicator::~STM32Communicator() {
    StopMonitoring();
    Disconnect();
}

bool STM32Communicator::Connect(const std::string& port_name, int baud_rate) {
    std::lock_guard<std::mutex> lock(serial_mutex_);

    if (is_connected_) {
        SetError("Already connected. Disconnect first.");
        return false;
    }

    SerialConfig config;
    config.baud_rate = baud_rate;
    config.data_bits = 8;
    config.stop_bits = 1;
    config.parity = 'E';  // STM32 bootloader uses even parity
    config.timeout_ms = 1000;

    if (!serial_.Open(port_name, config)) {
        SetError("Failed to open port: " + serial_.GetLastError());
        return false;
    }

    port_name_ = port_name;
    baud_rate_ = baud_rate;
    is_connected_ = true;

    return true;
}

void STM32Communicator::Disconnect() {
    StopMonitoring();

    std::lock_guard<std::mutex> lock(serial_mutex_);

    if (is_connected_) {
        serial_.Close();
        is_connected_ = false;
        port_name_.clear();
    }
}

bool STM32Communicator::IsConnected() const {
    return is_connected_;
}

std::string STM32Communicator::GetPortName() const {
    return port_name_;
}

bool STM32Communicator::EnterBootloader(bool pulse_dtr) {
    std::lock_guard<std::mutex> lock(serial_mutex_);

    if (!is_connected_) {
        SetError("Not connected to any port");
        return false;
    }

    // Pulse DTR to reset the MCU if requested
    if (pulse_dtr) {
        if (!serial_.PulseDTR(100, true)) {
            SetError("Failed to pulse DTR: " + serial_.GetLastError());
            return false;
        }
        // Wait for MCU to boot
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Flush any existing data
    serial_.Flush();

    // Send initialization byte
    uint8_t init_byte = 0x7F;
    if (serial_.Write(&init_byte, 1) != 1) {
        SetError("Failed to send init byte");
        return false;
    }

    // Wait for ACK
    if (!WaitForAck(1000)) {
        SetError("No ACK received from bootloader");
        return false;
    }

    return true;
}

bool STM32Communicator::Flash(const FirmwareData& firmware, bool erase_all) {
    std::lock_guard<std::mutex> lock(serial_mutex_);

    if (!is_connected_) {
        SetError("Not connected to any port");
        return false;
    }

    if (firmware.data.empty()) {
        SetError("Firmware data is empty");
        return false;
    }

    // Erase memory
    std::cout << "Erasing flash memory..." << std::endl;
    if (!EraseMemory(erase_all)) {
        SetError("Failed to erase memory");
        return false;
    }

    // Write firmware in chunks
    const size_t CHUNK_SIZE = 256;  // STM32 bootloader typically supports up to 256 bytes
    size_t total_written = 0;
    uint32_t address = firmware.start_address;

    std::cout << "Writing " << firmware.data.size() << " bytes to 0x"
              << std::hex << address << std::dec << "..." << std::endl;

    while (total_written < firmware.data.size()) {
        size_t remaining = firmware.data.size() - total_written;
        size_t chunk_size = (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE;

        if (!WriteMemory(address, firmware.data.data() + total_written, chunk_size)) {
            SetError("Failed to write memory at address 0x" +
                    std::to_string(address));
            return false;
        }

        total_written += chunk_size;
        address += chunk_size;

        // Print progress
        int progress = (total_written * 100) / firmware.data.size();
        std::cout << "\rProgress: " << progress << "%" << std::flush;
    }

    std::cout << "\nFlashing completed successfully!" << std::endl;
    return true;
}

bool STM32Communicator::StartMonitoring(DataCallback callback) {
    if (monitoring_active_) {
        SetError("Monitoring already active");
        return false;
    }

    if (!is_connected_) {
        SetError("Not connected to any port");
        return false;
    }

    data_callback_ = callback;
    monitoring_active_ = true;
    monitor_thread_ = std::thread(&STM32Communicator::MonitorThreadFunc, this);

    return true;
}

void STM32Communicator::StopMonitoring() {
    if (monitoring_active_) {
        monitoring_active_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }
}

bool STM32Communicator::IsMonitoring() const {
    return monitoring_active_;
}

int STM32Communicator::Send(const uint8_t* data, size_t length) {
    std::lock_guard<std::mutex> lock(serial_mutex_);

    if (!is_connected_) {
        SetError("Not connected to any port");
        return -1;
    }

    return serial_.Write(data, length);
}

int STM32Communicator::Send(const std::string& str) {
    return Send(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
}

int STM32Communicator::Send(const std::vector<uint8_t>& data) {
    return Send(data.data(), data.size());
}

std::string STM32Communicator::GetLastError() const {
    return last_error_;
}

void STM32Communicator::SetError(const std::string& error) {
    last_error_ = error;
}

void STM32Communicator::MonitorThreadFunc() {
    const size_t BUFFER_SIZE = 1024;
    uint8_t buffer[BUFFER_SIZE];

    while (monitoring_active_) {
        int bytes_read;
        {
            std::lock_guard<std::mutex> lock(serial_mutex_);
            if (!is_connected_) {
                break;
            }
            bytes_read = serial_.Read(buffer, BUFFER_SIZE);
        }

        if (bytes_read > 0) {
            if (data_callback_) {
                // Call user callback
                data_callback_(buffer, bytes_read);
            } else {
                // Default: print to stdout
                std::cout.write(reinterpret_cast<const char*>(buffer), bytes_read);
                std::cout.flush();
            }
        } else if (bytes_read < 0) {
            // Error occurred
            std::cerr << "Error reading from serial: " << serial_.GetLastError() << std::endl;
            break;
        }
        // bytes_read == 0 means timeout, continue

        // Small delay to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool STM32Communicator::SendCommand(BootloaderCommand cmd) {
    uint8_t cmd_bytes[2];
    cmd_bytes[0] = static_cast<uint8_t>(cmd);
    cmd_bytes[1] = ~cmd_bytes[0];  // Complement

    if (serial_.Write(cmd_bytes, 2) != 2) {
        return false;
    }

    return WaitForAck();
}

bool STM32Communicator::SendCommandWithAddress(BootloaderCommand cmd, uint32_t address) {
    // Send command
    if (!SendCommand(cmd)) {
        return false;
    }

    // Send address with checksum
    uint8_t addr_bytes[5];
    addr_bytes[0] = (address >> 24) & 0xFF;
    addr_bytes[1] = (address >> 16) & 0xFF;
    addr_bytes[2] = (address >> 8) & 0xFF;
    addr_bytes[3] = address & 0xFF;
    addr_bytes[4] = addr_bytes[0] ^ addr_bytes[1] ^ addr_bytes[2] ^ addr_bytes[3];

    if (serial_.Write(addr_bytes, 5) != 5) {
        return false;
    }

    return WaitForAck();
}

bool STM32Communicator::WaitForAck(int timeout_ms) {
    auto start = std::chrono::steady_clock::now();

    while (true) {
        uint8_t response;
        int bytes_read = serial_.Read(&response, 1);

        if (bytes_read == 1) {
            if (response == static_cast<uint8_t>(BootloaderResponse::ACK)) {
                return true;
            } else if (response == static_cast<uint8_t>(BootloaderResponse::NACK)) {
                return false;
            }
        }

        auto elapsed = std::chrono::steady_clock::now() - start;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= timeout_ms) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool STM32Communicator::WriteMemory(uint32_t address, const uint8_t* data, size_t length) {
    if (length == 0 || length > 256) {
        return false;
    }

    // Send Write Memory command with address
    if (!SendCommandWithAddress(BootloaderCommand::WRITE_MEMORY, address)) {
        return false;
    }

    // Prepare data packet: N-1 (1 byte) + data (N bytes) + checksum (1 byte)
    std::vector<uint8_t> packet;
    packet.push_back(static_cast<uint8_t>(length - 1));  // N-1
    packet.insert(packet.end(), data, data + length);

    // Calculate checksum (XOR of all bytes)
    uint8_t checksum = packet[0];
    for (size_t i = 1; i < packet.size(); i++) {
        checksum ^= packet[i];
    }
    packet.push_back(checksum);

    // Send packet
    if (serial_.Write(packet.data(), packet.size()) != static_cast<int>(packet.size())) {
        return false;
    }

    return WaitForAck();
}

bool STM32Communicator::EraseMemory(bool full_erase) {
    // Send Extended Erase command (0x44)
    uint8_t cmd_bytes[2];
    cmd_bytes[0] = static_cast<uint8_t>(BootloaderCommand::EXTENDED_ERASE);
    cmd_bytes[1] = ~cmd_bytes[0];

    if (serial_.Write(cmd_bytes, 2) != 2) {
        return false;
    }

    if (!WaitForAck()) {
        return false;
    }

    if (full_erase) {
        // Global erase: 0xFFFF + checksum
        uint8_t erase_cmd[3] = {0xFF, 0xFF, 0x00};
        if (serial_.Write(erase_cmd, 3) != 3) {
            return false;
        }
    } else {
        // For now, only support global erase
        // Sector-specific erase would require sector list
        uint8_t erase_cmd[3] = {0xFF, 0xFF, 0x00};
        if (serial_.Write(erase_cmd, 3) != 3) {
            return false;
        }
    }

    // Erase can take a long time, use extended timeout
    return WaitForAck(30000);  // 30 second timeout for erase
}

uint8_t STM32Communicator::CalculateChecksum(const uint8_t* data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

} // namespace SimpleSerial
