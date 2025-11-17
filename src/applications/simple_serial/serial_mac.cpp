#include "serial.h"
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <dirent.h>
#include <errno.h>

namespace SimpleSerial {

Serial::Serial()
    : fd_(-1)
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

    is_open_ = true;
    return true;
}

void Serial::Close() {
    if (!is_open_) {
        return;
    }

    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
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

    ssize_t result = write(fd_, data, length);
    if (result < 0) {
        SetError("Write failed: " + std::string(strerror(errno)));
        return -1;
    }
    return result;
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

    int bytes_available;
    if (ioctl(fd_, FIONREAD, &bytes_available) < 0) {
        return -1;
    }
    return bytes_available;
}

bool Serial::Flush() {
    if (!is_open_) {
        return false;
    }

    return tcflush(fd_, TCIOFLUSH) == 0;
}

std::string Serial::GetLastError() const {
    return last_error_;
}

bool Serial::SetDTR(bool state) {
    if (!is_open_) {
        SetError("Serial port not open");
        return false;
    }

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
}

bool Serial::SetRTS(bool state) {
    if (!is_open_) {
        SetError("Serial port not open");
        return false;
    }

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
}

int Serial::GetDTR() const {
    if (!is_open_) {
        return -1;
    }

    int status;
    if (ioctl(fd_, TIOCMGET, &status) < 0) {
        return -1;
    }
    return (status & TIOCM_DTR) ? 1 : 0;
}

int Serial::GetRTS() const {
    if (!is_open_) {
        return -1;
    }

    int status;
    if (ioctl(fd_, TIOCMGET, &status) < 0) {
        return -1;
    }
    return (status & TIOCM_RTS) ? 1 : 0;
}

bool Serial::SetControlLines(bool dtr, bool rts) {
    if (!is_open_) {
        SetError("Serial port not open");
        return false;
    }

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
    usleep(duration_ms * 1000);

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
    usleep(duration_ms * 1000);

    // Return to initial state
    if (!SetRTS(initial_state)) {
        return false;
    }

    return true;
}

std::vector<std::string> Serial::ListPorts() {
    std::vector<std::string> ports;

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

    return ports;
}

bool Serial::ConfigurePort() {
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

    return true;
}

void Serial::SetError(const std::string& error) {
    last_error_ = error;
}

} // namespace SimpleSerial
