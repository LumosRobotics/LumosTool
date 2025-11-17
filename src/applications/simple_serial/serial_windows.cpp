#include "serial.h"
#include <cstring>
#include <iostream>
#include <windows.h>

namespace SimpleSerial {

Serial::Serial()
    : handle_(INVALID_HANDLE_VALUE)
    , is_open_(false)
{
}

Serial::~Serial() {
    Close();
}

bool Serial::Open(const std::string& port_name, const SerialConfig& config) {
    if (is_open_) {
        SetError("Serial port already open");
        return false;
    }

    port_name_ = port_name;
    config_ = config;

    // Open the serial port
    handle_ = CreateFileA(
        port_name.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (handle_ == INVALID_HANDLE_VALUE) {
        SetError("Failed to open serial port: " + port_name);
        return false;
    }

    if (!ConfigurePort()) {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
        return false;
    }

    is_open_ = true;
    return true;
}

void Serial::Close() {
    if (!is_open_) {
        return;
    }

    if (handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }

    is_open_ = false;
}

bool Serial::IsOpen() const {
    return is_open_;
}

int Serial::Write(const uint8_t* data, size_t length) {
    if (!is_open_) {
        SetError("Serial port not open");
        return -1;
    }

    DWORD bytes_written;
    if (!WriteFile(handle_, data, length, &bytes_written, NULL)) {
        SetError("Write failed");
        return -1;
    }
    return bytes_written;
}

int Serial::Write(const std::string& str) {
    return Write(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
}

int Serial::Write(const std::vector<uint8_t>& data) {
    return Write(data.data(), data.size());
}

int Serial::Read(uint8_t* buffer, size_t max_length) {
    if (!is_open_) {
        SetError("Serial port not open");
        return -1;
    }

    DWORD bytes_read;
    if (!ReadFile(handle_, buffer, max_length, &bytes_read, NULL)) {
        SetError("Read failed");
        return -1;
    }
    return bytes_read;
}

std::vector<uint8_t> Serial::Read(size_t max_length) {
    std::vector<uint8_t> buffer(max_length);
    int bytes_read = Read(buffer.data(), max_length);

    if (bytes_read <= 0) {
        return std::vector<uint8_t>();
    }

    buffer.resize(bytes_read);
    return buffer;
}

std::vector<uint8_t> Serial::ReadUntil(uint8_t terminator, size_t max_length) {
    std::vector<uint8_t> result;
    uint8_t byte;

    while (result.size() < max_length) {
        int bytes_read = Read(&byte, 1);

        if (bytes_read <= 0) {
            break;
        }

        result.push_back(byte);

        if (byte == terminator) {
            break;
        }
    }

    return result;
}

std::string Serial::ReadLine(size_t max_length) {
    auto data = ReadUntil('\n', max_length);

    if (data.empty()) {
        return "";
    }

    // Remove trailing \r\n if present
    while (!data.empty() && (data.back() == '\n' || data.back() == '\r')) {
        data.pop_back();
    }

    return std::string(data.begin(), data.end());
}

int Serial::Available() const {
    if (!is_open_) {
        return -1;
    }

    COMSTAT status;
    DWORD errors;
    if (!ClearCommError(handle_, &errors, &status)) {
        return -1;
    }
    return status.cbInQue;
}

bool Serial::Flush() {
    if (!is_open_) {
        return false;
    }

    return FlushFileBuffers(handle_) != 0;
}

std::string Serial::GetLastError() const {
    return last_error_;
}

bool Serial::SetDTR(bool state) {
    if (!is_open_) {
        SetError("Serial port not open");
        return false;
    }

    DWORD func = state ? SETDTR : CLRDTR;
    if (!EscapeCommFunction(handle_, func)) {
        SetError("Failed to set DTR");
        return false;
    }
    return true;
}

bool Serial::SetRTS(bool state) {
    if (!is_open_) {
        SetError("Serial port not open");
        return false;
    }

    DWORD func = state ? SETRTS : CLRRTS;
    if (!EscapeCommFunction(handle_, func)) {
        SetError("Failed to set RTS");
        return false;
    }
    return true;
}

int Serial::GetDTR() const {
    if (!is_open_) {
        return -1;
    }

    DWORD modem_status;
    if (!GetCommModemStatus(handle_, &modem_status)) {
        return -1;
    }
    // Note: On Windows, we can't directly read DTR state we set
    // This returns the DTR output state based on DCB settings
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(handle_, &dcb)) {
        return -1;
    }
    return dcb.fDtrControl == DTR_CONTROL_ENABLE ? 1 : 0;
}

int Serial::GetRTS() const {
    if (!is_open_) {
        return -1;
    }

    DWORD modem_status;
    if (!GetCommModemStatus(handle_, &modem_status)) {
        return -1;
    }
    // Note: On Windows, we can't directly read RTS state we set
    // This returns the RTS output state based on DCB settings
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(handle_, &dcb)) {
        return -1;
    }
    return dcb.fRtsControl == RTS_CONTROL_ENABLE ? 1 : 0;
}

bool Serial::SetControlLines(bool dtr, bool rts) {
    if (!is_open_) {
        SetError("Serial port not open");
        return false;
    }

    // Set DTR
    DWORD dtr_func = dtr ? SETDTR : CLRDTR;
    if (!EscapeCommFunction(handle_, dtr_func)) {
        SetError("Failed to set DTR");
        return false;
    }

    // Set RTS
    DWORD rts_func = rts ? SETRTS : CLRRTS;
    if (!EscapeCommFunction(handle_, rts_func)) {
        SetError("Failed to set RTS");
        return false;
    }

    return true;
}

bool Serial::PulseDTR(int duration_ms, bool active_low) {
    if (!is_open_) {
        SetError("Serial port not open");
        return false;
    }

    // Determine initial and pulsed states based on active_low
    bool initial_state = active_low ? true : false;
    bool pulsed_state = !initial_state;

    // Set to pulsed state
    if (!SetDTR(pulsed_state)) {
        return false;
    }

    // Wait for specified duration
    Sleep(duration_ms);

    // Return to initial state
    if (!SetDTR(initial_state)) {
        return false;
    }

    return true;
}

bool Serial::PulseRTS(int duration_ms, bool active_low) {
    if (!is_open_) {
        SetError("Serial port not open");
        return false;
    }

    // Determine initial and pulsed states based on active_low
    bool initial_state = active_low ? true : false;
    bool pulsed_state = !initial_state;

    // Set to pulsed state
    if (!SetRTS(pulsed_state)) {
        return false;
    }

    // Wait for specified duration
    Sleep(duration_ms);

    // Return to initial state
    if (!SetRTS(initial_state)) {
        return false;
    }

    return true;
}

std::vector<std::string> Serial::ListPorts() {
    std::vector<std::string> ports;

    // Windows: Check COM1 through COM256
    for (int i = 1; i <= 256; i++) {
        std::string port = "COM" + std::to_string(i);
        HANDLE h = CreateFileA(
            port.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (h != INVALID_HANDLE_VALUE) {
            ports.push_back(port);
            CloseHandle(h);
        }
    }

    return ports;
}

bool Serial::ConfigurePort() {
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(handle_, &dcb)) {
        SetError("Failed to get comm state");
        return false;
    }

    dcb.BaudRate = config_.baud_rate;
    dcb.ByteSize = config_.data_bits;
    dcb.StopBits = (config_.stop_bits == 1) ? ONESTOPBIT : TWOSTOPBITS;

    switch (config_.parity) {
        case 'N': dcb.Parity = NOPARITY; break;
        case 'E': dcb.Parity = EVENPARITY; break;
        case 'O': dcb.Parity = ODDPARITY; break;
        default: dcb.Parity = NOPARITY; break;
    }

    if (!SetCommState(handle_, &dcb)) {
        SetError("Failed to set comm state");
        return false;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = config_.timeout_ms;
    timeouts.ReadTotalTimeoutConstant = config_.timeout_ms;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = config_.timeout_ms;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    if (!SetCommTimeouts(handle_, &timeouts)) {
        SetError("Failed to set timeouts");
        return false;
    }

    return true;
}

void Serial::SetError(const std::string& error) {
    last_error_ = error;
}

} // namespace SimpleSerial
