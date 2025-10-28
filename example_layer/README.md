# STM32 UART C++ Wrapper

This example demonstrates a modern C++ wrapper layer for the STM32 UART peripheral, designed to simplify and modernize embedded UART communication while maintaining performance and safety.

## Overview

The `UartWrapper` class provides a high-level, type-safe interface to STM32 UART peripherals with the following key features:

### Key Features

- **Modern C++ Design**: Uses C++20 features like `std::span`, `std::string_view`, and `std::optional`
- **RAII Resource Management**: Automatic initialization and cleanup
- **Type Safety**: Strong typing prevents common embedded programming errors
- **Multiple Operation Modes**: Blocking, interrupt-based, and DMA-based operations
- **Comprehensive Error Handling**: Detailed error reporting and callbacks
- **Factory Methods**: Easy creation of common UART configurations
- **Callback Support**: Asynchronous operation callbacks for non-blocking code

### Design Philosophy

This wrapper addresses common pain points in embedded UART programming:

1. **Manual Resource Management**: Automatic clock enabling, GPIO configuration, and cleanup
2. **Error-Prone Configuration**: Type-safe configuration structs prevent mistakes
3. **Inconsistent APIs**: Unified interface for blocking, interrupt, and DMA operations
4. **Poor Error Handling**: Comprehensive error types and callback mechanisms
5. **C-style Memory Management**: Modern C++ memory safety without dynamic allocation

## Architecture

### Class Structure

```cpp
namespace lumos {
    class UartWrapper {
        // Configuration and lifecycle
        bool initialize(const UartConfig& config);
        void deinitialize();
        
        // Blocking operations
        bool send(std::span<const uint8_t> data, uint32_t timeout_ms = 1000);
        bool send(std::string_view str, uint32_t timeout_ms = 1000);
        size_t receive(std::span<uint8_t> buffer, uint32_t timeout_ms = 1000);
        
        // Asynchronous operations
        bool send_async(std::span<const uint8_t> data, TxCompleteCallback callback = nullptr);
        bool receive_async(std::span<uint8_t> buffer, RxCompleteCallback callback = nullptr);
        
        // DMA operations
        bool send_dma(std::span<const uint8_t> data, TxCompleteCallback callback = nullptr);
        bool receive_dma(std::span<uint8_t> buffer, RxCompleteCallback callback = nullptr);
        
        // Factory methods
        static UartWrapper create_usart3(uint32_t baudrate = 115200);
    };
}
```

### Configuration System

The wrapper uses a comprehensive configuration structure:

```cpp
struct UartConfig {
    // UART parameters
    uint32_t baudrate = 115200;
    uint32_t word_length = UART_WORDLENGTH_8B;
    uint32_t stop_bits = UART_STOPBITS_1;
    uint32_t parity = UART_PARITY_NONE;
    uint32_t hw_flow_control = UART_HWCONTROL_NONE;
    
    // GPIO configuration
    GPIO_TypeDef* tx_port;
    uint16_t tx_pin;
    uint8_t tx_alternate_function;
    
    // Advanced features
    bool enable_fifo = false;
    uint32_t tx_timeout_ms = 1000;
    uint32_t rx_timeout_ms = 1000;
};
```

### Error Handling

The wrapper provides comprehensive error handling:

```cpp
enum class UartError {
    None, Timeout, Overrun, Framing, 
    Noise, Parity, DMA, Busy, 
    InvalidParameter, HardwareFault
};
```

## Usage Examples

### 1. Simple Usage with Factory Method

```cpp
#include "uart_wrapper.hpp"

int main() {
    HAL_Init();
    SystemClock_Config();
    
    // Create UART with default configuration
    auto uart = lumos::UartWrapper::create_usart3();
    
    if (!uart.is_ready()) {
        return -1; // Handle error
    }
    
    // Send messages
    uart.send("Hello World!\r\n");
    uart.send_formatted("System Clock: %lu MHz\r\n", 
                       HAL_RCC_GetSysClockFreq() / 1000000);
    
    // Echo loop
    std::array<uint8_t, 64> buffer{};
    while (true) {
        size_t received = uart.receive_until(buffer, '\r', 5000);
        if (received > 0) {
            uart.send("Echo: ");
            uart.send(std::span<const uint8_t>(buffer.data(), received));
            uart.send("\r\n");
        }
    }
}
```

### 2. Custom Configuration

```cpp
lumos::UartWrapper uart(USART1);

lumos::UartConfig config;
config.baudrate = 9600;
config.parity = UART_PARITY_EVEN;
config.stop_bits = UART_STOPBITS_2;
config.hw_flow_control = UART_HWCONTROL_RTS_CTS;

// Configure GPIO
config.tx_port = GPIOA;
config.tx_pin = GPIO_PIN_9;
config.tx_alternate_function = GPIO_AF7_USART1;

config.rx_port = GPIOA;
config.rx_pin = GPIO_PIN_10;
config.rx_alternate_function = GPIO_AF7_USART1;

if (!uart.initialize(config)) {
    auto error = uart.get_last_error();
    // Handle specific error
}
```

### 3. Asynchronous Operations with Callbacks

```cpp
auto uart = lumos::UartWrapper::create_usart2();

// Set error callback
uart.set_error_callback([](lumos::UartError error) {
    printf("UART Error: %d\r\n", static_cast<int>(error));
});

// Async transmission
std::string message = "Async message!\r\n";
uart.send_async(message, []() {
    printf("Transmission completed!\r\n");
});

// Async reception
std::array<uint8_t, 32> rx_buffer{};
uart.receive_async(rx_buffer, [](std::span<const uint8_t> data) {
    printf("Received %zu bytes\r\n", data.size());
});
```

### 4. DMA Operations

```cpp
auto uart = lumos::UartWrapper::create_uart4();

// Large buffer for DMA
std::array<uint8_t, 1024> large_buffer{};

// Fill with test data
std::iota(large_buffer.begin(), large_buffer.end(), 0);

// Send using DMA
uart.send_dma(large_buffer, []() {
    printf("DMA transmission completed!\r\n");
});

// Wait for completion
uart.flush();
```

### 5. Error Handling and State Management

```cpp
auto uart = lumos::UartWrapper::create_uart5();

std::array<uint8_t, 16> buffer{};
size_t received = uart.receive(buffer, 1000);

if (received == 0) {
    auto error = uart.get_last_error();
    switch (error) {
        case lumos::UartError::Timeout:
            uart.send("Timeout occurred\r\n");
            break;
        case lumos::UartError::Overrun:
            uart.send("Buffer overrun\r\n");
            break;
        default:
            uart.send_formatted("Error: %d\r\n", static_cast<int>(error));
            break;
    }
}

// Check UART state
auto state = uart.get_state();
if (state != lumos::UartState::Ready) {
    // Handle non-ready state
}
```

## Benefits Over Raw HAL

### 1. **Safety**
- Type-safe configuration prevents runtime errors
- RAII ensures proper resource cleanup
- Comprehensive error handling with specific error types

### 2. **Usability**
- Intuitive API with modern C++ conventions
- Factory methods for common configurations
- Consistent interface across operation modes

### 3. **Performance**
- Zero-cost abstractions - no runtime overhead
- Direct HAL integration for maximum performance
- Efficient memory usage without dynamic allocation

### 4. **Maintainability**
- Clear separation of concerns
- Extensible design for additional features
- Self-documenting code with type safety

## Integration with Build System

The wrapper integrates with the existing Lumos build system:

```cmake
# Add to your CMakeLists.txt
target_sources(your_project PRIVATE
    example_layer/uart_wrapper.cpp
)

target_include_directories(your_project PRIVATE
    example_layer
)
```

## Extending the Wrapper

The design is easily extensible:

### Adding New Peripherals
```cpp
// Similar wrappers can be created for other peripherals
class SpiWrapper { /* ... */ };
class I2cWrapper { /* ... */ };
class CanWrapper { /* ... */ };
```

### Adding Features
```cpp
// Extend UartWrapper with additional features
class UartWrapper {
    // Add circular buffer support
    bool enable_circular_buffer(size_t size);
    
    // Add protocol support
    bool send_packet(const Packet& packet);
    
    // Add flow control
    bool set_flow_control(bool enable);
};
```

## Memory Usage

The wrapper is designed for minimal memory overhead:

- **Static Memory**: Only what's needed for UART handle and configuration
- **No Dynamic Allocation**: All buffers are stack-allocated or provided by user
- **Compile-Time Optimization**: Template functions and constexpr where possible

## Thread Safety

Current implementation considerations:

- **Single Threaded**: Designed for typical embedded single-threaded applications
- **Interrupt Safe**: Callback mechanisms work with interrupt-driven operations
- **Future Enhancement**: Can be extended with mutex protection for multi-threaded use

## Testing and Validation

Recommended testing approaches:

1. **Unit Tests**: Test configuration validation and error handling
2. **Hardware-in-Loop**: Test with actual STM32H7 hardware
3. **Loopback Tests**: Connect TX to RX for self-testing
4. **Stress Tests**: High-frequency operations and error injection

## Future Enhancements

Potential improvements:

1. **Circular Buffer Support**: Built-in buffering for continuous operations
2. **Protocol Layers**: Built-in support for common protocols (Modbus, etc.)
3. **Power Management**: Integration with STM32 low-power modes
4. **Multi-Instance**: Better support for multiple UART instances
5. **Template Specialization**: Compile-time optimization for specific configurations

## Conclusion

This C++ wrapper demonstrates how modern C++ techniques can significantly improve embedded system development while maintaining the performance characteristics required for real-time applications. The design principles shown here can be applied to other STM32 peripherals to create a comprehensive, type-safe embedded framework.