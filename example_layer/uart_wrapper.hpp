#pragma once

#include "stm32h7xx_hal.h"
#include <cstdint>
#include <cstring>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

namespace lumos {

/**
 * @brief UART Configuration structure
 */
struct UartConfig {
    // Basic UART parameters
    uint32_t baudrate = 115200;
    uint32_t word_length = UART_WORDLENGTH_8B;
    uint32_t stop_bits = UART_STOPBITS_1;
    uint32_t parity = UART_PARITY_NONE;
    uint32_t mode = UART_MODE_TX_RX;
    uint32_t hw_flow_control = UART_HWCONTROL_NONE;
    uint32_t oversampling = UART_OVERSAMPLING_16;
    bool one_bit_sampling = false;
    uint32_t clock_prescaler = UART_PRESCALER_DIV1;
    
    // GPIO configuration
    GPIO_TypeDef* tx_port = nullptr;
    uint16_t tx_pin = 0;
    uint8_t tx_alternate_function = 0;
    
    GPIO_TypeDef* rx_port = nullptr;
    uint16_t rx_pin = 0;
    uint8_t rx_alternate_function = 0;
    
    // Optional RTS/CTS pins for hardware flow control
    GPIO_TypeDef* rts_port = nullptr;
    uint16_t rts_pin = 0;
    uint8_t rts_alternate_function = 0;
    
    GPIO_TypeDef* cts_port = nullptr;
    uint16_t cts_pin = 0;
    uint8_t cts_alternate_function = 0;
    
    // Timeout settings
    uint32_t tx_timeout_ms = 1000;
    uint32_t rx_timeout_ms = 1000;
    
    // FIFO configuration
    bool enable_fifo = false;
    uint32_t tx_fifo_threshold = UART_TXFIFO_THRESHOLD_1_8;
    uint32_t rx_fifo_threshold = UART_RXFIFO_THRESHOLD_1_8;
};

/**
 * @brief UART Error codes
 */
enum class UartError {
    None = 0,
    Timeout,
    Overrun,
    Framing,
    Noise,
    Parity,
    DMA,
    Busy,
    InvalidParameter,
    HardwareFault
};

/**
 * @brief UART State
 */
enum class UartState {
    Reset = 0,
    Ready,
    Busy,
    BusyTx,
    BusyRx,
    BusyTxRx,
    Error
};

/**
 * @brief Callback function types
 */
using TxCompleteCallback = std::function<void()>;
using RxCompleteCallback = std::function<void(std::span<const uint8_t> data)>;
using ErrorCallback = std::function<void(UartError error)>;

/**
 * @brief Modern C++ wrapper for STM32 UART peripheral
 * 
 * This class provides a high-level, type-safe interface to the STM32 UART
 * peripheral while maintaining performance characteristics suitable for
 * embedded systems.
 * 
 * Features:
 * - RAII resource management
 * - Type-safe configuration
 * - Modern C++ interfaces (string_view, span, optional)
 * - Interrupt and DMA support
 * - Comprehensive error handling
 * - Callback-based asynchronous operations
 */
class UartWrapper {
public:
    /**
     * @brief Constructor
     * @param instance UART instance (e.g., USART1, USART2, etc.)
     */
    explicit UartWrapper(USART_TypeDef* instance);
    
    /**
     * @brief Destructor - automatically deinitializes UART
     */
    ~UartWrapper();
    
    // Non-copyable but movable
    UartWrapper(const UartWrapper&) = delete;
    UartWrapper& operator=(const UartWrapper&) = delete;
    UartWrapper(UartWrapper&& other) noexcept;
    UartWrapper& operator=(UartWrapper&& other) noexcept;
    
    /**
     * @brief Initialize UART with given configuration
     * @param config UART configuration
     * @return true if successful, false otherwise
     */
    bool initialize(const UartConfig& config);
    
    /**
     * @brief Deinitialize UART and release resources
     */
    void deinitialize();
    
    /**
     * @brief Check if UART is initialized and ready
     * @return true if ready, false otherwise
     */
    bool is_ready() const;
    
    /**
     * @brief Get current UART state
     * @return Current state
     */
    UartState get_state() const;
    
    /**
     * @brief Get last error
     * @return Last error code
     */
    UartError get_last_error() const;
    
    // === Blocking Transmission Methods ===
    
    /**
     * @brief Send data synchronously
     * @param data Data to send
     * @param timeout_ms Timeout in milliseconds
     * @return true if successful, false on error/timeout
     */
    bool send(std::span<const uint8_t> data, uint32_t timeout_ms = 1000);
    
    /**
     * @brief Send string synchronously
     * @param str String to send
     * @param timeout_ms Timeout in milliseconds
     * @return true if successful, false on error/timeout
     */
    bool send(std::string_view str, uint32_t timeout_ms = 1000);
    
    /**
     * @brief Send formatted string synchronously
     * @param format Format string (printf-style)
     * @param args Format arguments
     * @return true if successful, false on error/timeout
     */
    template<typename... Args>
    bool send_formatted(const char* format, Args&&... args);
    
    /**
     * @brief Receive data synchronously
     * @param buffer Buffer to store received data
     * @param timeout_ms Timeout in milliseconds
     * @return Number of bytes received, or 0 on error/timeout
     */
    size_t receive(std::span<uint8_t> buffer, uint32_t timeout_ms = 1000);
    
    /**
     * @brief Receive data until delimiter or timeout
     * @param buffer Buffer to store received data
     * @param delimiter Delimiter character
     * @param timeout_ms Timeout in milliseconds
     * @return Number of bytes received, or 0 on error/timeout
     */
    size_t receive_until(std::span<uint8_t> buffer, char delimiter = '\n', uint32_t timeout_ms = 1000);
    
    // === Non-blocking (Interrupt-based) Transmission Methods ===
    
    /**
     * @brief Send data asynchronously using interrupts
     * @param data Data to send
     * @param callback Callback to call when transmission is complete
     * @return true if transmission started, false otherwise
     */
    bool send_async(std::span<const uint8_t> data, TxCompleteCallback callback = nullptr);
    
    /**
     * @brief Send string asynchronously using interrupts
     * @param str String to send
     * @param callback Callback to call when transmission is complete
     * @return true if transmission started, false otherwise
     */
    bool send_async(std::string_view str, TxCompleteCallback callback = nullptr);
    
    /**
     * @brief Receive data asynchronously using interrupts
     * @param buffer Buffer to store received data
     * @param callback Callback to call when reception is complete
     * @return true if reception started, false otherwise
     */
    bool receive_async(std::span<uint8_t> buffer, RxCompleteCallback callback = nullptr);
    
    // === DMA-based Transmission Methods ===
    
    /**
     * @brief Send data using DMA
     * @param data Data to send
     * @param callback Callback to call when transmission is complete
     * @return true if DMA transmission started, false otherwise
     */
    bool send_dma(std::span<const uint8_t> data, TxCompleteCallback callback = nullptr);
    
    /**
     * @brief Receive data using DMA
     * @param buffer Buffer to store received data
     * @param callback Callback to call when reception is complete
     * @return true if DMA reception started, false otherwise
     */
    bool receive_dma(std::span<uint8_t> buffer, RxCompleteCallback callback = nullptr);
    
    // === Control Methods ===
    
    /**
     * @brief Abort ongoing transmission
     * @return true if successful, false otherwise
     */
    bool abort_transmit();
    
    /**
     * @brief Abort ongoing reception
     * @return true if successful, false otherwise
     */
    bool abort_receive();
    
    /**
     * @brief Abort all ongoing operations
     * @return true if successful, false otherwise
     */
    bool abort_all();
    
    /**
     * @brief Set error callback
     * @param callback Callback to call on errors
     */
    void set_error_callback(ErrorCallback callback);
    
    /**
     * @brief Enable/disable RX interrupt for continuous reception
     * @param enable true to enable, false to disable
     * @return true if successful, false otherwise
     */
    bool enable_rx_interrupt(bool enable = true);
    
    // === Utility Methods ===
    
    /**
     * @brief Get number of bytes available in RX buffer (if using interrupts)
     * @return Number of available bytes
     */
    size_t bytes_available() const;
    
    /**
     * @brief Flush TX buffer (wait for transmission to complete)
     * @param timeout_ms Timeout in milliseconds
     * @return true if successful, false on timeout
     */
    bool flush(uint32_t timeout_ms = 1000);
    
    /**
     * @brief Clear RX buffer
     */
    void clear_rx_buffer();
    
    /**
     * @brief Get UART handle for direct HAL access (use with caution)
     * @return Pointer to UART handle
     */
    UART_HandleTypeDef* get_handle() { return &uart_handle_; }
    
    // === Static Factory Methods ===
    
    /**
     * @brief Create UART wrapper with common STM32H7 configurations
     */
    static UartWrapper create_usart1(uint32_t baudrate = 115200);
    static UartWrapper create_usart2(uint32_t baudrate = 115200);
    static UartWrapper create_usart3(uint32_t baudrate = 115200);
    static UartWrapper create_uart4(uint32_t baudrate = 115200);
    static UartWrapper create_uart5(uint32_t baudrate = 115200);
    
private:
    // Private methods
    void configure_gpio(const UartConfig& config);
    void enable_clocks();
    void disable_clocks();
    UartError hal_error_to_uart_error(HAL_StatusTypeDef hal_status) const;
    void handle_error(UartError error);
    
    // Static interrupt handlers (friends)
    friend void uart_tx_complete_callback(UART_HandleTypeDef* huart);
    friend void uart_rx_complete_callback(UART_HandleTypeDef* huart);
    friend void uart_error_callback(UART_HandleTypeDef* huart);
    
    // Member variables
    UART_HandleTypeDef uart_handle_{};
    UartConfig config_{};
    UartError last_error_{UartError::None};
    bool initialized_{false};
    
    // Callbacks
    TxCompleteCallback tx_complete_callback_;
    RxCompleteCallback rx_complete_callback_;
    ErrorCallback error_callback_;
    
    // Buffers for async operations
    static constexpr size_t INTERNAL_BUFFER_SIZE = 256;
    uint8_t internal_tx_buffer_[INTERNAL_BUFFER_SIZE];
    uint8_t internal_rx_buffer_[INTERNAL_BUFFER_SIZE];
    
    // Static instance tracking for interrupt handling
    static UartWrapper* instances_[8]; // Support up to 8 UART instances
    static size_t get_instance_index(USART_TypeDef* instance);
};

// === Convenience aliases ===
using Uart = UartWrapper;

} // namespace lumos