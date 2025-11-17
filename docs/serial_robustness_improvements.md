# Serial Communication Robustness Improvements

This document outlines potential improvements to increase robustness and decrease connection/disconnection troubles in the serial communication module.

## 1. Port Filtering (macOS-specific issue)

**Problem**: On macOS, you see both `/dev/cu.*` and `/dev/tty.*` for the same device
- `cu.*` = "call-up" (outgoing connections) - **preferred**
- `tty.*` = "teletype" (incoming connections with modem control)

**Fix**: Filter to show only `cu.*` ports on macOS

```cpp
// In serial_mac.cpp ListPorts()
if (name.find("cu.") == 0) {  // Only cu.*, skip tty.*
    ports.push_back("/dev/" + name);
}
```

**Impact**: Easy fix, high impact - reduces port confusion and improves connection reliability

---

## 2. Exclusive Lock Detection

**Problem**: Port might be in use by another application, leading to cryptic errors

**Fix**:
- Use `flock()` or check `errno == EBUSY` on POSIX
- Provide clear error message: "Port in use by another application"

```cpp
bool Serial::Open(const std::string& port_name, const SerialConfig& config) {
    // ... existing open code ...

    if (fd_ == -1) {
        if (errno == EBUSY) {
            SetError("Port is in use by another application");
        } else if (errno == EACCES) {
            SetError("Permission denied. On Linux, add user to 'dialout' group: "
                    "sudo usermod -a -G dialout $USER");
        } else {
            SetError("Failed to open serial port: " + port_name +
                    " (" + std::string(strerror(errno)) + ")");
        }
        return false;
    }
}
```

**Impact**: Medium effort, improves error messages significantly

---

## 3. Flush on Open

**Problem**: Stale data in buffers from previous sessions can corrupt communication

**Fix**: Always flush both RX and TX buffers immediately after opening

```cpp
bool Serial::Open(const std::string& port_name, const SerialConfig& config) {
    // ... existing open code ...

    // Double-flush to ensure clean buffers
    tcflush(fd_, TCIOFLUSH);
    usleep(50000);            // 50ms delay for device to settle
    tcflush(fd_, TCIOFLUSH);  // Flush again

    is_open_ = true;
    return true;
}
```

**Impact**: Easy fix, prevents weird communication bugs

---

## 4. DTR/RTS State Management

**Problem**: DTR/RTS lines can trigger reset on connection, disrupting normal operation

**Fix**: Add option to control DTR/RTS on open

```cpp
struct SerialConfig {
    int baud_rate = 115200;
    int data_bits = 8;
    int stop_bits = 1;
    char parity = 'N';
    int timeout_ms = 1000;

    // New options
    bool set_dtr_on_open = false;  // Don't assert DTR by default
    bool set_rts_on_open = false;  // Don't assert RTS by default
};

bool Serial::Open(const std::string& port_name, const SerialConfig& config) {
    // ... after configuring port ...

    // Set initial DTR/RTS state
    if (config.set_dtr_on_open) {
        SetDTR(true);
    }
    if (config.set_rts_on_open) {
        SetRTS(true);
    }
}
```

**Impact**: Medium effort, important for bootloader mode and device compatibility

---

## 5. Disconnect Detection

**Problem**: Device unplugged mid-operation causes hangs or confusing errors

**Fix**: Check for disconnect errors and close port cleanly

```cpp
int Serial::Read(uint8_t* buffer, size_t max_length) {
    // ... existing read code ...

    ssize_t result = read(fd_, buffer, max_length);
    if (result < 0) {
        if (errno == ENXIO || errno == EIO) {
            SetError("Device disconnected");
            Close();
            return -1;
        }
        SetError("Read failed: " + std::string(strerror(errno)));
        return -1;
    }

    return result;
}

int Serial::Write(const uint8_t* data, size_t length) {
    // Similar disconnect detection
    ssize_t result = write(fd_, data, length);
    if (result < 0) {
        if (errno == ENXIO || errno == EIO) {
            SetError("Device disconnected");
            Close();
            return -1;
        }
        // ... existing error handling
    }
}
```

**Impact**: Medium effort, prevents hangs and improves error handling

---

## 6. Port Validation Before Open

**Problem**: Trying to open non-existent port wastes time

**Fix**: Quick check before opening

```cpp
bool Serial::IsPortAvailable(const std::string& port) {
    struct stat st;
    return (stat(port.c_str(), &st) == 0);
}

bool Serial::Open(const std::string& port_name, const SerialConfig& config) {
    if (!IsPortAvailable(port_name)) {
        SetError("Port does not exist: " + port_name);
        return false;
    }

    // ... rest of open logic
}
```

**Impact**: Easy fix, faster failure feedback

---

## 7. Retry Logic with Exponential Backoff

**Problem**: Transient errors (device busy, buffer full) fail immediately

**Fix**: Retry mechanism with exponential backoff

```cpp
bool Serial::Open(const std::string& port_name, const SerialConfig& config,
                  int max_retries = 3) {
    int retry_delay_ms = 100;

    for (int attempt = 0; attempt < max_retries; attempt++) {
        if (attempt > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
            retry_delay_ms *= 2;  // Exponential backoff: 100ms, 200ms, 400ms
        }

        fd_ = open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

        if (fd_ != -1) {
            // Success, continue with configuration
            break;
        }

        if (errno != EBUSY && attempt < max_retries - 1) {
            // Not a transient error, don't retry
            break;
        }
    }

    if (fd_ == -1) {
        SetError("Failed to open port after retries: " + std::string(strerror(errno)));
        return false;
    }

    // ... rest of open logic
}
```

**Impact**: Medium effort, handles transient errors gracefully

---

## 8. Read with Total Timeout

**Problem**: Current timeout is per-read operation, can hang indefinitely on slow data

**Fix**: Track total elapsed time across multiple reads

```cpp
std::vector<uint8_t> Serial::ReadUntil(uint8_t terminator, size_t max_length,
                                       int total_timeout_ms = 5000) {
    std::vector<uint8_t> result;
    uint8_t byte;

    auto start_time = std::chrono::steady_clock::now();

    while (result.size() < max_length) {
        // Check total elapsed time
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

        if (elapsed_ms >= total_timeout_ms) {
            SetError("Total timeout exceeded");
            break;
        }

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
```

**Impact**: Medium effort, prevents indefinite hangs

---

## 9. Better Error Messages

**Problem**: Generic "Failed to open port" doesn't help debugging

**Fix**: Specific error codes and messages

```cpp
enum class SerialError {
    SUCCESS,
    PORT_NOT_FOUND,
    PORT_IN_USE,
    PERMISSION_DENIED,
    DEVICE_DISCONNECTED,
    TIMEOUT,
    INVALID_CONFIG,
    UNKNOWN_ERROR
};

class Serial {
public:
    // ... existing methods ...

    SerialError GetLastErrorCode() const { return last_error_code_; }
    std::string GetLastError() const { return last_error_; }

private:
    SerialError last_error_code_;
    std::string last_error_;

    void SetError(const std::string& error, SerialError code) {
        last_error_ = error;
        last_error_code_ = code;
    }
};
```

**Impact**: Easy fix, greatly improves user experience

---

## 10. Permission Detection (Linux)

**Problem**: User lacks permission to access `/dev/ttyUSB0`

**Fix**: Check permissions and suggest solution

```cpp
bool Serial::Open(const std::string& port_name, const SerialConfig& config) {
    // ... existing code ...

    if (fd_ == -1) {
        if (errno == EACCES) {
            #ifdef __linux__
            SetError("Permission denied. Add your user to the 'dialout' group:\n"
                    "  sudo usermod -a -G dialout $USER\n"
                    "Then log out and log back in.");
            #else
            SetError("Permission denied: " + port_name);
            #endif
        }
        return false;
    }
}
```

**Impact**: Easy fix, helps new users troubleshoot

---

## 11. Auto-Reconnect on Monitor

**Problem**: If device disconnects during monitoring, thread crashes

**Fix**: Monitor thread detects disconnect and attempts reconnect

```cpp
void STM32Communicator::MonitorThreadFunc() {
    const size_t BUFFER_SIZE = 1024;
    uint8_t buffer[BUFFER_SIZE];
    int consecutive_errors = 0;
    const int MAX_CONSECUTIVE_ERRORS = 5;

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
            consecutive_errors = 0;  // Reset error counter

            if (data_callback_) {
                data_callback_(buffer, bytes_read);
            } else {
                std::cout.write(reinterpret_cast<const char*>(buffer), bytes_read);
                std::cout.flush();
            }
        } else if (bytes_read < 0) {
            consecutive_errors++;

            if (consecutive_errors >= MAX_CONSECUTIVE_ERRORS) {
                std::cerr << "Error reading from serial: " << serial_.GetLastError() << std::endl;
                std::cerr << "Attempting to reconnect..." << std::endl;

                // Try to reconnect
                Disconnect();
                std::this_thread::sleep_for(std::chrono::seconds(1));

                if (Connect(port_name_, baud_rate_)) {
                    std::cout << "Reconnected successfully!" << std::endl;
                    consecutive_errors = 0;
                } else {
                    std::cerr << "Reconnect failed. Stopping monitor." << std::endl;
                    break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
```

**Impact**: Medium-high effort, significantly improves monitoring reliability

---

## 12. Hardware Flow Control Handling

**Problem**: Some devices expect hardware flow control (RTS/CTS)

**Fix**: Make it configurable

```cpp
struct SerialConfig {
    int baud_rate = 115200;
    int data_bits = 8;
    int stop_bits = 1;
    char parity = 'N';
    int timeout_ms = 1000;
    bool set_dtr_on_open = false;
    bool set_rts_on_open = false;

    // New option
    bool hardware_flow_control = false;  // Currently hardcoded to disabled
};

bool Serial::ConfigurePort() {
    // ... existing configuration ...

    // Hardware flow control
    if (config_.hardware_flow_control) {
        tty.c_cflag |= CRTSCTS;  // Enable RTS/CTS
    } else {
        tty.c_cflag &= ~CRTSCTS;  // Disable (default)
    }

    // ... rest of configuration
}
```

**Impact**: Easy fix, improves device compatibility

---

## Priority Recommendations

If implementing improvements in order of impact:

### High Priority (Easy + High Impact)
1. **Port filtering on macOS** - Reduces confusion, improves reliability
2. **Flush on open** - Prevents weird communication bugs
3. **Better error messages** - Greatly improves UX
4. **Port validation** - Faster failure feedback

### Medium Priority (Important for Reliability)
5. **DTR/RTS control** - Important for bootloader mode
6. **Disconnect detection** - Prevents hangs
7. **Permission detection** - Helps troubleshooting

### Lower Priority (Nice to Have)
8. **Retry logic** - Handles transient errors
9. **Total timeout** - Prevents indefinite hangs
10. **Auto-reconnect** - Improves monitoring
11. **Hardware flow control** - Device compatibility
12. **Exclusive lock detection** - Better error messages

---

## Implementation Notes

- Most improvements are backward compatible
- Can be implemented incrementally
- Each improvement should include unit tests
- Consider adding integration tests with loopback serial devices
- Document breaking changes in SerialConfig structure
