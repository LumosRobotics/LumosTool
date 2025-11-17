#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace SimpleSerial {

/**
 * @brief Serial port configuration
 */
struct SerialConfig {
    int baud_rate = 115200;
    int data_bits = 8;
    int stop_bits = 1;
    char parity = 'N';  // 'N' = None, 'E' = Even, 'O' = Odd
    int timeout_ms = 1000;  // Read timeout in milliseconds
};

/**
 * @brief Cross-platform serial port communication class
 *
 * Provides a simple interface for serial port communication across different platforms.
 * Supports basic operations like opening, closing, reading, and writing to serial ports.
 */
class Serial {
public:
    Serial();
    ~Serial();

    /**
     * @brief Open a serial port
     * @param port_name Name of the serial port (e.g., "/dev/ttyUSB0" on Linux, "COM3" on Windows)
     * @param config Serial port configuration (baud rate, data bits, etc.)
     * @return true if successful, false otherwise
     */
    bool Open(const std::string& port_name, const SerialConfig& config = SerialConfig());

    /**
     * @brief Close the serial port
     */
    void Close();

    /**
     * @brief Check if the serial port is open
     * @return true if open, false otherwise
     */
    bool IsOpen() const;

    /**
     * @brief Write data to the serial port
     * @param data Pointer to data buffer
     * @param length Number of bytes to write
     * @return Number of bytes actually written, -1 on error
     */
    int Write(const uint8_t* data, size_t length);

    /**
     * @brief Write a string to the serial port
     * @param str String to write
     * @return Number of bytes written, -1 on error
     */
    int Write(const std::string& str);

    /**
     * @brief Write a vector of bytes to the serial port
     * @param data Vector of bytes to write
     * @return Number of bytes written, -1 on error
     */
    int Write(const std::vector<uint8_t>& data);

    /**
     * @brief Read data from the serial port
     * @param buffer Pointer to buffer to store read data
     * @param max_length Maximum number of bytes to read
     * @return Number of bytes actually read, -1 on error, 0 on timeout
     */
    int Read(uint8_t* buffer, size_t max_length);

    /**
     * @brief Read data into a vector
     * @param max_length Maximum number of bytes to read
     * @return Vector containing read data (empty on error or timeout)
     */
    std::vector<uint8_t> Read(size_t max_length);

    /**
     * @brief Read until a specific byte is encountered or max_length is reached
     * @param terminator Byte to stop reading at (inclusive)
     * @param max_length Maximum number of bytes to read
     * @return Vector containing read data including terminator (empty on error)
     */
    std::vector<uint8_t> ReadUntil(uint8_t terminator, size_t max_length = 1024);

    /**
     * @brief Read a line (until '\n' or max_length)
     * @param max_length Maximum number of bytes to read
     * @return String containing the line (without newline), empty on error
     */
    std::string ReadLine(size_t max_length = 1024);

    /**
     * @brief Get the number of bytes available to read
     * @return Number of bytes available, -1 on error
     */
    int Available() const;

    /**
     * @brief Flush the serial port buffers
     * @return true if successful, false otherwise
     */
    bool Flush();

    /**
     * @brief Get the last error message
     * @return Error message string
     */
    std::string GetLastError() const;

    /**
     * @brief Set DTR (Data Terminal Ready) line state
     * @param state true = assert DTR, false = clear DTR
     * @return true if successful, false otherwise
     */
    bool SetDTR(bool state);

    /**
     * @brief Set RTS (Request To Send) line state
     * @param state true = assert RTS, false = clear RTS
     * @return true if successful, false otherwise
     */
    bool SetRTS(bool state);

    /**
     * @brief Get current DTR line state
     * @return true if DTR is asserted, false otherwise, -1 on error
     */
    int GetDTR() const;

    /**
     * @brief Get current RTS line state
     * @return true if RTS is asserted, false otherwise, -1 on error
     */
    int GetRTS() const;

    /**
     * @brief Set both DTR and RTS lines simultaneously
     * @param dtr DTR state
     * @param rts RTS state
     * @return true if successful, false otherwise
     */
    bool SetControlLines(bool dtr, bool rts);

    /**
     * @brief Pulse DTR line (useful for resetting MCU)
     * @param duration_ms Duration to hold DTR in pulsed state (milliseconds)
     * @param active_low true if reset is active-low (default), false if active-high
     * @return true if successful, false otherwise
     */
    bool PulseDTR(int duration_ms = 100, bool active_low = true);

    /**
     * @brief Pulse RTS line
     * @param duration_ms Duration to hold RTS in pulsed state (milliseconds)
     * @param active_low true if active-low (default), false if active-high
     * @return true if successful, false otherwise
     */
    bool PulseRTS(int duration_ms = 100, bool active_low = true);

    /**
     * @brief List available serial ports on the system
     * @return Vector of port names
     */
    static std::vector<std::string> ListPorts();

private:
    // Platform-specific handle
#ifdef _WIN32
    void* handle_;  // HANDLE on Windows
#else
    int fd_;  // File descriptor on POSIX systems
#endif

    SerialConfig config_;
    std::string port_name_;
    std::string last_error_;
    bool is_open_;

    // Platform-specific helper methods
    bool ConfigurePort();
    void SetError(const std::string& error);
};

} // namespace SimpleSerial
