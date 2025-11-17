#include "serial.h"
#include <cstring>
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <termios.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <dirent.h>
    #include <errno.h>
#endif

namespace SimpleSerial {

Serial::Serial()
#ifdef _WIN32
    : handle_(INVALID_HANDLE_VALUE)
#else
    : fd_(-1)
#endif
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

#ifdef _WIN32
    // Windows implementation
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

#else
    // POSIX implementation (macOS, Linux)
    fd_ = open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

    if (fd_ == -1) {
        SetError("Failed to open serial port: " + port_name + " (" + std::string(strerror(errno)) + ")");
        return false;
    }

    // Configure the port
    if (!ConfigurePort()) {
        close(fd_);
        fd_ = -1;
        return false;
    }

    // Set non-blocking mode
    fcntl(fd_, F_SETFL, 0);
#endif

    is_open_ = true;
    return true;
}

void Serial::Close() {
    if (!is_open_) {
        return;
    }

#ifdef _WIN32
    if (handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
#else
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }
#endif

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

#ifdef _WIN32
    DWORD bytes_written;
    if (!WriteFile(handle_, data, length, &bytes_written, NULL)) {
        SetError("Write failed");
        return -1;
    }
    return bytes_written;
#else
    ssize_t result = write(fd_, data, length);
    if (result < 0) {
        SetError("Write failed: " + std::string(strerror(errno)));
        return -1;
    }
    return result;
#endif
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

#ifdef _WIN32
    DWORD bytes_read;
    if (!ReadFile(handle_, buffer, max_length, &bytes_read, NULL)) {
        SetError("Read failed");
        return -1;
    }
    return bytes_read;
#else
    // Use select() for timeout
    fd_set read_fds;
    struct timeval timeout;

    FD_ZERO(&read_fds);
    FD_SET(fd_, &read_fds);

    timeout.tv_sec = config_.timeout_ms / 1000;
    timeout.tv_usec = (config_.timeout_ms % 1000) * 1000;

    int select_result = select(fd_ + 1, &read_fds, NULL, NULL, &timeout);

    if (select_result < 0) {
        SetError("Select failed: " + std::string(strerror(errno)));
        return -1;
    }

    if (select_result == 0) {
        // Timeout
        return 0;
    }

    ssize_t result = read(fd_, buffer, max_length);
    if (result < 0) {
        SetError("Read failed: " + std::string(strerror(errno)));
        return -1;
    }

    return result;
#endif
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

#ifdef _WIN32
    COMSTAT status;
    DWORD errors;
    if (!ClearCommError(handle_, &errors, &status)) {
        return -1;
    }
    return status.cbInQue;
#else
    int bytes_available;
    if (ioctl(fd_, FIONREAD, &bytes_available) < 0) {
        return -1;
    }
    return bytes_available;
#endif
}

bool Serial::Flush() {
    if (!is_open_) {
        return false;
    }

#ifdef _WIN32
    return FlushFileBuffers(handle_) != 0;
#else
    return tcflush(fd_, TCIOFLUSH) == 0;
#endif
}

std::string Serial::GetLastError() const {
    return last_error_;
}

std::vector<std::string> Serial::ListPorts() {
    std::vector<std::string> ports;

#ifdef _WIN32
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
#elif __APPLE__
    // macOS: Check /dev/tty.* and /dev/cu.*
    DIR* dir = opendir("/dev");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            if (name.find("tty.") == 0 || name.find("cu.") == 0) {
                ports.push_back("/dev/" + name);
            }
        }
        closedir(dir);
    }
#else
    // Linux: Check /dev/ttyUSB*, /dev/ttyACM*, /dev/ttyS*
    DIR* dir = opendir("/dev");
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            if (name.find("ttyUSB") == 0 ||
                name.find("ttyACM") == 0 ||
                name.find("ttyS") == 0) {
                ports.push_back("/dev/" + name);
            }
        }
        closedir(dir);
    }
#endif

    return ports;
}

bool Serial::ConfigurePort() {
#ifdef _WIN32
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

#else
    // POSIX configuration
    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(fd_, &tty) != 0) {
        SetError("Failed to get terminal attributes: " + std::string(strerror(errno)));
        return false;
    }

    // Set baud rate
    speed_t baud;
    switch (config_.baud_rate) {
        case 9600: baud = B9600; break;
        case 19200: baud = B19200; break;
        case 38400: baud = B38400; break;
        case 57600: baud = B57600; break;
        case 115200: baud = B115200; break;
        case 230400: baud = B230400; break;
        default:
            SetError("Unsupported baud rate: " + std::to_string(config_.baud_rate));
            return false;
    }

    cfsetispeed(&tty, baud);
    cfsetospeed(&tty, baud);

    // Set data bits
    tty.c_cflag &= ~CSIZE;
    switch (config_.data_bits) {
        case 5: tty.c_cflag |= CS5; break;
        case 6: tty.c_cflag |= CS6; break;
        case 7: tty.c_cflag |= CS7; break;
        case 8: tty.c_cflag |= CS8; break;
        default:
            SetError("Unsupported data bits: " + std::to_string(config_.data_bits));
            return false;
    }

    // Set parity
    switch (config_.parity) {
        case 'N':
            tty.c_cflag &= ~PARENB;
            break;
        case 'E':
            tty.c_cflag |= PARENB;
            tty.c_cflag &= ~PARODD;
            break;
        case 'O':
            tty.c_cflag |= PARENB;
            tty.c_cflag |= PARODD;
            break;
        default:
            SetError("Unsupported parity: " + std::string(1, config_.parity));
            return false;
    }

    // Set stop bits
    if (config_.stop_bits == 1) {
        tty.c_cflag &= ~CSTOPB;
    } else if (config_.stop_bits == 2) {
        tty.c_cflag |= CSTOPB;
    } else {
        SetError("Unsupported stop bits: " + std::to_string(config_.stop_bits));
        return false;
    }

    // Control flags
    tty.c_cflag |= (CLOCAL | CREAD);  // Enable receiver, ignore modem control lines

    // Disable hardware flow control
    tty.c_cflag &= ~CRTSCTS;

    // Input flags - disable software flow control
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    // Disable canonical mode, echo, and signals
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // Output flags - disable output processing
    tty.c_oflag &= ~OPOST;

    // Set timeout (deciseconds)
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = config_.timeout_ms / 100;

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
        SetError("Failed to set terminal attributes: " + std::string(strerror(errno)));
        return false;
    }
#endif

    return true;
}

void Serial::SetError(const std::string& error) {
    last_error_ = error;
}

bool Serial::SetDTR(bool state) {
    if (!is_open_) {
        SetError("Serial port not open");
        return false;
    }

#ifdef _WIN32
    DWORD func = state ? SETDTR : CLRDTR;
    if (!EscapeCommFunction(handle_, func)) {
        SetError("Failed to set DTR");
        return false;
    }
    return true;
#else
    int status;
    if (ioctl(fd_, TIOCMGET, &status) < 0) {
        SetError("Failed to get modem status: " + std::string(strerror(errno)));
        return false;
    }

    if (state) {
        status |= TIOCM_DTR;  // Set DTR
    } else {
        status &= ~TIOCM_DTR; // Clear DTR
    }

    if (ioctl(fd_, TIOCMSET, &status) < 0) {
        SetError("Failed to set DTR: " + std::string(strerror(errno)));
        return false;
    }

    return true;
#endif
}

bool Serial::SetRTS(bool state) {
    if (!is_open_) {
        SetError("Serial port not open");
        return false;
    }

#ifdef _WIN32
    DWORD func = state ? SETRTS : CLRRTS;
    if (!EscapeCommFunction(handle_, func)) {
        SetError("Failed to set RTS");
        return false;
    }
    return true;
#else
    int status;
    if (ioctl(fd_, TIOCMGET, &status) < 0) {
        SetError("Failed to get modem status: " + std::string(strerror(errno)));
        return false;
    }

    if (state) {
        status |= TIOCM_RTS;  // Set RTS
    } else {
        status &= ~TIOCM_RTS; // Clear RTS
    }

    if (ioctl(fd_, TIOCMSET, &status) < 0) {
        SetError("Failed to set RTS: " + std::string(strerror(errno)));
        return false;
    }

    return true;
#endif
}

int Serial::GetDTR() const {
    if (!is_open_) {
        return -1;
    }

#ifdef _WIN32
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
#else
    int status;
    if (ioctl(fd_, TIOCMGET, &status) < 0) {
        return -1;
    }
    return (status & TIOCM_DTR) ? 1 : 0;
#endif
}

int Serial::GetRTS() const {
    if (!is_open_) {
        return -1;
    }

#ifdef _WIN32
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
#else
    int status;
    if (ioctl(fd_, TIOCMGET, &status) < 0) {
        return -1;
    }
    return (status & TIOCM_RTS) ? 1 : 0;
#endif
}

bool Serial::SetControlLines(bool dtr, bool rts) {
    if (!is_open_) {
        SetError("Serial port not open");
        return false;
    }

#ifdef _WIN32
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
#else
    int status;
    if (ioctl(fd_, TIOCMGET, &status) < 0) {
        SetError("Failed to get modem status: " + std::string(strerror(errno)));
        return false;
    }

    // Set or clear DTR
    if (dtr) {
        status |= TIOCM_DTR;
    } else {
        status &= ~TIOCM_DTR;
    }

    // Set or clear RTS
    if (rts) {
        status |= TIOCM_RTS;
    } else {
        status &= ~TIOCM_RTS;
    }

    if (ioctl(fd_, TIOCMSET, &status) < 0) {
        SetError("Failed to set control lines: " + std::string(strerror(errno)));
        return false;
    }

    return true;
#endif
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
#ifdef _WIN32
    Sleep(duration_ms);
#else
    usleep(duration_ms * 1000);
#endif

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
#ifdef _WIN32
    Sleep(duration_ms);
#else
    usleep(duration_ms * 1000);
#endif

    // Return to initial state
    if (!SetRTS(initial_state)) {
        return false;
    }

    return true;
}

} // namespace SimpleSerial
