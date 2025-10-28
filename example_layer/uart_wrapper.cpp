#include "uart_wrapper.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdarg>

namespace lumos
{

    // Static instance tracking
    UartWrapper *UartWrapper::instances_[8] = {nullptr};

    UartWrapper::UartWrapper(USART_TypeDef *instance)
    {
        uart_handle_.Instance = instance;

        // Register this instance for interrupt handling
        size_t index = get_instance_index(instance);
        if (index < 8)
        {
            instances_[index] = this;
        }
    }

    UartWrapper::~UartWrapper()
    {
        deinitialize();

        // Unregister this instance
        size_t index = get_instance_index(uart_handle_.Instance);
        if (index < 8)
        {
            instances_[index] = nullptr;
        }
    }

    UartWrapper::UartWrapper(UartWrapper &&other) noexcept
        : uart_handle_(other.uart_handle_), config_(other.config_), last_error_(other.last_error_), initialized_(other.initialized_), tx_complete_callback_(std::move(other.tx_complete_callback_)), rx_complete_callback_(std::move(other.rx_complete_callback_)), error_callback_(std::move(other.error_callback_))
    {
        // Update instance tracking
        size_t index = get_instance_index(uart_handle_.Instance);
        if (index < 8)
        {
            instances_[index] = this;
        }

        // Reset other object
        other.uart_handle_ = {};
        other.initialized_ = false;
    }

    UartWrapper &UartWrapper::operator=(UartWrapper &&other) noexcept
    {
        if (this != &other)
        {
            // Deinitialize current instance
            deinitialize();

            // Move data
            uart_handle_ = other.uart_handle_;
            config_ = other.config_;
            last_error_ = other.last_error_;
            initialized_ = other.initialized_;
            tx_complete_callback_ = std::move(other.tx_complete_callback_);
            rx_complete_callback_ = std::move(other.rx_complete_callback_);
            error_callback_ = std::move(other.error_callback_);

            // Update instance tracking
            size_t index = get_instance_index(uart_handle_.Instance);
            if (index < 8)
            {
                instances_[index] = this;
            }

            // Reset other object
            other.uart_handle_ = {};
            other.initialized_ = false;
        }
        return *this;
    }

    bool UartWrapper::initialize(const UartConfig &config)
    {
        if (initialized_)
        {
            deinitialize();
        }

        config_ = config;

        // Enable clocks
        enable_clocks();

        // Configure GPIO
        configure_gpio(config);

        // Configure UART parameters
        uart_handle_.Init.BaudRate = config.baudrate;
        uart_handle_.Init.WordLength = config.word_length;
        uart_handle_.Init.StopBits = config.stop_bits;
        uart_handle_.Init.Parity = config.parity;
        uart_handle_.Init.Mode = config.mode;
        uart_handle_.Init.HwFlowCtl = config.hw_flow_control;
        uart_handle_.Init.OverSampling = config.oversampling;
        uart_handle_.Init.OneBitSampling = config.one_bit_sampling ? UART_ONE_BIT_SAMPLE_ENABLE : UART_ONE_BIT_SAMPLE_DISABLE;
        uart_handle_.Init.ClockPrescaler = config.clock_prescaler;
        uart_handle_.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

        // Initialize UART
        HAL_StatusTypeDef status = HAL_UART_Init(&uart_handle_);
        if (status != HAL_OK)
        {
            last_error_ = hal_error_to_uart_error(status);
            return false;
        }

        // Configure FIFO if enabled
        if (config.enable_fifo)
        {
            if (HAL_UARTEx_SetTxFifoThreshold(&uart_handle_, config.tx_fifo_threshold) != HAL_OK ||
                HAL_UARTEx_SetRxFifoThreshold(&uart_handle_, config.rx_fifo_threshold) != HAL_OK ||
                HAL_UARTEx_EnableFifoMode(&uart_handle_) != HAL_OK)
            {
                last_error_ = UartError::HardwareFault;
                return false;
            }
        }
        else
        {
            HAL_UARTEx_DisableFifoMode(&uart_handle_);
        }

        initialized_ = true;
        last_error_ = UartError::None;
        return true;
    }

    void UartWrapper::deinitialize()
    {
        if (initialized_)
        {
            HAL_UART_DeInit(&uart_handle_);
            disable_clocks();
            initialized_ = false;
        }
    }

    bool UartWrapper::is_ready() const
    {
        return initialized_ && (get_state() == UartState::Ready);
    }

    UartState UartWrapper::get_state() const
    {
        if (!initialized_)
        {
            return UartState::Reset;
        }

        switch (uart_handle_.gState)
        {
        case HAL_UART_STATE_RESET:
            return UartState::Reset;
        case HAL_UART_STATE_READY:
            return UartState::Ready;
        case HAL_UART_STATE_BUSY:
            return UartState::Busy;
        case HAL_UART_STATE_BUSY_TX:
            return UartState::BusyTx;
        case HAL_UART_STATE_BUSY_RX:
            return UartState::BusyRx;
        case HAL_UART_STATE_BUSY_TX_RX:
            return UartState::BusyTxRx;
        case HAL_UART_STATE_ERROR:
            return UartState::Error;
        default:
            return UartState::Error;
        }
    }

    UartError UartWrapper::get_last_error() const
    {
        return last_error_;
    }

    bool UartWrapper::send(std::span<const uint8_t> data, uint32_t timeout_ms)
    {
        if (!initialized_ || data.empty())
        {
            last_error_ = UartError::InvalidParameter;
            return false;
        }

        HAL_StatusTypeDef status = HAL_UART_Transmit(&uart_handle_,
                                                     const_cast<uint8_t *>(data.data()),
                                                     data.size(),
                                                     timeout_ms);

        if (status != HAL_OK)
        {
            last_error_ = hal_error_to_uart_error(status);
            return false;
        }

        last_error_ = UartError::None;
        return true;
    }

    bool UartWrapper::send(std::string_view str, uint32_t timeout_ms)
    {
        std::span<const uint8_t> data(reinterpret_cast<const uint8_t *>(str.data()), str.size());
        return send(data, timeout_ms);
    }

    template <typename... Args>
    bool UartWrapper::send_formatted(const char *format, Args &&...args)
    {
        int len = std::snprintf(reinterpret_cast<char *>(internal_tx_buffer_),
                                INTERNAL_BUFFER_SIZE,
                                format,
                                std::forward<Args>(args)...);

        if (len < 0 || static_cast<size_t>(len) >= INTERNAL_BUFFER_SIZE)
        {
            last_error_ = UartError::InvalidParameter;
            return false;
        }

        std::span<const uint8_t> data(internal_tx_buffer_, len);
        return send(data, config_.tx_timeout_ms);
    }

    size_t UartWrapper::receive(std::span<uint8_t> buffer, uint32_t timeout_ms)
    {
        if (!initialized_ || buffer.empty())
        {
            last_error_ = UartError::InvalidParameter;
            return 0;
        }

        HAL_StatusTypeDef status = HAL_UART_Receive(&uart_handle_,
                                                    buffer.data(),
                                                    buffer.size(),
                                                    timeout_ms);

        if (status != HAL_OK)
        {
            last_error_ = hal_error_to_uart_error(status);
            return 0;
        }

        last_error_ = UartError::None;
        return buffer.size();
    }

    size_t UartWrapper::receive_until(std::span<uint8_t> buffer, char delimiter, uint32_t timeout_ms)
    {
        if (!initialized_ || buffer.empty())
        {
            last_error_ = UartError::InvalidParameter;
            return 0;
        }

        size_t bytes_received = 0;
        uint32_t start_time = HAL_GetTick();

        while (bytes_received < buffer.size())
        {
            // Check timeout
            if (HAL_GetTick() - start_time > timeout_ms)
            {
                last_error_ = UartError::Timeout;
                break;
            }

            // Try to receive one byte
            HAL_StatusTypeDef status = HAL_UART_Receive(&uart_handle_,
                                                        &buffer[bytes_received],
                                                        1,
                                                        10); // Short timeout for individual bytes

            if (status == HAL_OK)
            {
                if (buffer[bytes_received] == delimiter)
                {
                    bytes_received++;
                    break; // Found delimiter
                }
                bytes_received++;
            }
            else if (status != HAL_TIMEOUT)
            {
                last_error_ = hal_error_to_uart_error(status);
                break;
            }
        }

        if (last_error_ == UartError::None && bytes_received > 0)
        {
            return bytes_received;
        }

        return 0;
    }

    bool UartWrapper::send_async(std::span<const uint8_t> data, TxCompleteCallback callback)
    {
        if (!initialized_ || data.empty())
        {
            last_error_ = UartError::InvalidParameter;
            return false;
        }

        tx_complete_callback_ = callback;

        HAL_StatusTypeDef status = HAL_UART_Transmit_IT(&uart_handle_,
                                                        const_cast<uint8_t *>(data.data()),
                                                        data.size());

        if (status != HAL_OK)
        {
            last_error_ = hal_error_to_uart_error(status);
            tx_complete_callback_ = nullptr;
            return false;
        }

        last_error_ = UartError::None;
        return true;
    }

    bool UartWrapper::send_async(std::string_view str, TxCompleteCallback callback)
    {
        // Copy string to internal buffer to ensure it stays valid during async transmission
        if (str.size() >= INTERNAL_BUFFER_SIZE)
        {
            last_error_ = UartError::InvalidParameter;
            return false;
        }

        std::memcpy(internal_tx_buffer_, str.data(), str.size());
        std::span<const uint8_t> data(internal_tx_buffer_, str.size());

        return send_async(data, callback);
    }

    bool UartWrapper::receive_async(std::span<uint8_t> buffer, RxCompleteCallback callback)
    {
        if (!initialized_ || buffer.empty())
        {
            last_error_ = UartError::InvalidParameter;
            return false;
        }

        rx_complete_callback_ = callback;

        HAL_StatusTypeDef status = HAL_UART_Receive_IT(&uart_handle_,
                                                       buffer.data(),
                                                       buffer.size());

        if (status != HAL_OK)
        {
            last_error_ = hal_error_to_uart_error(status);
            rx_complete_callback_ = nullptr;
            return false;
        }

        last_error_ = UartError::None;
        return true;
    }

    bool UartWrapper::send_dma(std::span<const uint8_t> data, TxCompleteCallback callback)
    {
        if (!initialized_ || data.empty())
        {
            last_error_ = UartError::InvalidParameter;
            return false;
        }

        tx_complete_callback_ = callback;

        HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(&uart_handle_,
                                                         const_cast<uint8_t *>(data.data()),
                                                         data.size());

        if (status != HAL_OK)
        {
            last_error_ = hal_error_to_uart_error(status);
            tx_complete_callback_ = nullptr;
            return false;
        }

        last_error_ = UartError::None;
        return true;
    }

    bool UartWrapper::receive_dma(std::span<uint8_t> buffer, RxCompleteCallback callback)
    {
        if (!initialized_ || buffer.empty())
        {
            last_error_ = UartError::InvalidParameter;
            return false;
        }

        rx_complete_callback_ = callback;

        HAL_StatusTypeDef status = HAL_UART_Receive_DMA(&uart_handle_,
                                                        buffer.data(),
                                                        buffer.size());

        if (status != HAL_OK)
        {
            last_error_ = hal_error_to_uart_error(status);
            rx_complete_callback_ = nullptr;
            return false;
        }

        last_error_ = UartError::None;
        return true;
    }

    bool UartWrapper::abort_transmit()
    {
        if (!initialized_)
        {
            return false;
        }

        HAL_StatusTypeDef status = HAL_UART_AbortTransmit(&uart_handle_);
        return status == HAL_OK;
    }

    bool UartWrapper::abort_receive()
    {
        if (!initialized_)
        {
            return false;
        }

        HAL_StatusTypeDef status = HAL_UART_AbortReceive(&uart_handle_);
        return status == HAL_OK;
    }

    bool UartWrapper::abort_all()
    {
        if (!initialized_)
        {
            return false;
        }

        HAL_StatusTypeDef status = HAL_UART_Abort(&uart_handle_);
        return status == HAL_OK;
    }

    void UartWrapper::set_error_callback(ErrorCallback callback)
    {
        error_callback_ = callback;
    }

    bool UartWrapper::enable_rx_interrupt(bool enable)
    {
        if (!initialized_)
        {
            return false;
        }

        if (enable)
        {
            // Start continuous reception into internal buffer
            HAL_StatusTypeDef status = HAL_UART_Receive_IT(&uart_handle_,
                                                           internal_rx_buffer_,
                                                           1); // Receive one byte at a time
            return status == HAL_OK;
        }
        else
        {
            return abort_receive();
        }
    }

    size_t UartWrapper::bytes_available() const
    {
        // This would require a more sophisticated circular buffer implementation
        // For now, return 0 as this is a simplified example
        return 0;
    }

    bool UartWrapper::flush(uint32_t timeout_ms)
    {
        if (!initialized_)
        {
            return false;
        }

        uint32_t start_time = HAL_GetTick();

        // Wait for transmission to complete
        while (get_state() == UartState::BusyTx || get_state() == UartState::BusyTxRx)
        {
            if (HAL_GetTick() - start_time > timeout_ms)
            {
                return false;
            }
            HAL_Delay(1);
        }

        return true;
    }

    void UartWrapper::clear_rx_buffer()
    {
        // This would clear any internal circular buffer
        // For now, just abort any ongoing reception
        abort_receive();
    }

    // === Static Factory Methods ===

    UartWrapper UartWrapper::create_usart1(uint32_t baudrate)
    {
        UartWrapper uart(USART1);
        UartConfig config;
        config.baudrate = baudrate;
        config.tx_port = GPIOA;
        config.tx_pin = GPIO_PIN_9;
        config.tx_alternate_function = GPIO_AF7_USART1;
        config.rx_port = GPIOA;
        config.rx_pin = GPIO_PIN_10;
        config.rx_alternate_function = GPIO_AF7_USART1;

        uart.initialize(config);
        return uart;
    }

    UartWrapper UartWrapper::create_usart2(uint32_t baudrate)
    {
        UartWrapper uart(USART2);
        UartConfig config;
        config.baudrate = baudrate;
        config.tx_port = GPIOD;
        config.tx_pin = GPIO_PIN_5;
        config.tx_alternate_function = GPIO_AF7_USART2;
        config.rx_port = GPIOD;
        config.rx_pin = GPIO_PIN_6;
        config.rx_alternate_function = GPIO_AF7_USART2;

        uart.initialize(config);
        return uart;
    }

    UartWrapper UartWrapper::create_usart3(uint32_t baudrate)
    {
        UartWrapper uart(USART3);
        UartConfig config;
        config.baudrate = baudrate;
        config.tx_port = GPIOD;
        config.tx_pin = GPIO_PIN_8;
        config.tx_alternate_function = GPIO_AF7_USART3;
        config.rx_port = GPIOD;
        config.rx_pin = GPIO_PIN_9;
        config.rx_alternate_function = GPIO_AF7_USART3;

        uart.initialize(config);
        return uart;
    }

    UartWrapper UartWrapper::create_uart4(uint32_t baudrate)
    {
        UartWrapper uart(UART4);
        UartConfig config;
        config.baudrate = baudrate;
        config.tx_port = GPIOA;
        config.tx_pin = GPIO_PIN_0;
        config.tx_alternate_function = GPIO_AF8_UART4;
        config.rx_port = GPIOA;
        config.rx_pin = GPIO_PIN_1;
        config.rx_alternate_function = GPIO_AF8_UART4;

        uart.initialize(config);
        return uart;
    }

    UartWrapper UartWrapper::create_uart5(uint32_t baudrate)
    {
        UartWrapper uart(UART5);
        UartConfig config;
        config.baudrate = baudrate;
        config.tx_port = GPIOC;
        config.tx_pin = GPIO_PIN_12;
        config.tx_alternate_function = GPIO_AF8_UART5;
        config.rx_port = GPIOD;
        config.rx_pin = GPIO_PIN_2;
        config.rx_alternate_function = GPIO_AF8_UART5;

        uart.initialize(config);
        return uart;
    }

    // === Private Methods ===

    void UartWrapper::configure_gpio(const UartConfig &config)
    {
        GPIO_InitTypeDef gpio_init = {0};

        // Configure TX pin
        if (config.tx_port && config.tx_pin)
        {
            gpio_init.Pin = config.tx_pin;
            gpio_init.Mode = GPIO_MODE_AF_PP;
            gpio_init.Pull = GPIO_NOPULL;
            gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            gpio_init.Alternate = config.tx_alternate_function;
            HAL_GPIO_Init(config.tx_port, &gpio_init);
        }

        // Configure RX pin
        if (config.rx_port && config.rx_pin)
        {
            gpio_init.Pin = config.rx_pin;
            gpio_init.Mode = GPIO_MODE_AF_PP;
            gpio_init.Pull = GPIO_NOPULL;
            gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            gpio_init.Alternate = config.rx_alternate_function;
            HAL_GPIO_Init(config.rx_port, &gpio_init);
        }

        // Configure RTS pin if used
        if (config.hw_flow_control != UART_HWCONTROL_NONE && config.rts_port && config.rts_pin)
        {
            gpio_init.Pin = config.rts_pin;
            gpio_init.Mode = GPIO_MODE_AF_PP;
            gpio_init.Pull = GPIO_NOPULL;
            gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            gpio_init.Alternate = config.rts_alternate_function;
            HAL_GPIO_Init(config.rts_port, &gpio_init);
        }

        // Configure CTS pin if used
        if (config.hw_flow_control != UART_HWCONTROL_NONE && config.cts_port && config.cts_pin)
        {
            gpio_init.Pin = config.cts_pin;
            gpio_init.Mode = GPIO_MODE_AF_PP;
            gpio_init.Pull = GPIO_NOPULL;
            gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            gpio_init.Alternate = config.cts_alternate_function;
            HAL_GPIO_Init(config.cts_port, &gpio_init);
        }
    }

    void UartWrapper::enable_clocks()
    {
        // Enable GPIO clocks
        if (config_.tx_port == GPIOA || config_.rx_port == GPIOA)
            __HAL_RCC_GPIOA_CLK_ENABLE();
        if (config_.tx_port == GPIOB || config_.rx_port == GPIOB)
            __HAL_RCC_GPIOB_CLK_ENABLE();
        if (config_.tx_port == GPIOC || config_.rx_port == GPIOC)
            __HAL_RCC_GPIOC_CLK_ENABLE();
        if (config_.tx_port == GPIOD || config_.rx_port == GPIOD)
            __HAL_RCC_GPIOD_CLK_ENABLE();
        if (config_.tx_port == GPIOE || config_.rx_port == GPIOE)
            __HAL_RCC_GPIOE_CLK_ENABLE();
        if (config_.tx_port == GPIOF || config_.rx_port == GPIOF)
            __HAL_RCC_GPIOF_CLK_ENABLE();
        if (config_.tx_port == GPIOG || config_.rx_port == GPIOG)
            __HAL_RCC_GPIOG_CLK_ENABLE();
        if (config_.tx_port == GPIOH || config_.rx_port == GPIOH)
            __HAL_RCC_GPIOH_CLK_ENABLE();

        // Enable UART clocks
        if (uart_handle_.Instance == USART1)
            __HAL_RCC_USART1_CLK_ENABLE();
        else if (uart_handle_.Instance == USART2)
            __HAL_RCC_USART2_CLK_ENABLE();
        else if (uart_handle_.Instance == USART3)
            __HAL_RCC_USART3_CLK_ENABLE();
        else if (uart_handle_.Instance == UART4)
            __HAL_RCC_UART4_CLK_ENABLE();
        else if (uart_handle_.Instance == UART5)
            __HAL_RCC_UART5_CLK_ENABLE();
        else if (uart_handle_.Instance == USART6)
            __HAL_RCC_USART6_CLK_ENABLE();
        else if (uart_handle_.Instance == UART7)
            __HAL_RCC_UART7_CLK_ENABLE();
        else if (uart_handle_.Instance == UART8)
            __HAL_RCC_UART8_CLK_ENABLE();
    }

    void UartWrapper::disable_clocks()
    {
        // Disable UART clocks
        if (uart_handle_.Instance == USART1)
            __HAL_RCC_USART1_CLK_DISABLE();
        else if (uart_handle_.Instance == USART2)
            __HAL_RCC_USART2_CLK_DISABLE();
        else if (uart_handle_.Instance == USART3)
            __HAL_RCC_USART3_CLK_DISABLE();
        else if (uart_handle_.Instance == UART4)
            __HAL_RCC_UART4_CLK_DISABLE();
        else if (uart_handle_.Instance == UART5)
            __HAL_RCC_UART5_CLK_DISABLE();
        else if (uart_handle_.Instance == USART6)
            __HAL_RCC_USART6_CLK_DISABLE();
        else if (uart_handle_.Instance == UART7)
            __HAL_RCC_UART7_CLK_DISABLE();
        else if (uart_handle_.Instance == UART8)
            __HAL_RCC_UART8_CLK_DISABLE();
    }

    UartError UartWrapper::hal_error_to_uart_error(HAL_StatusTypeDef hal_status) const
    {
        switch (hal_status)
        {
        case HAL_OK:
            return UartError::None;
        case HAL_TIMEOUT:
            return UartError::Timeout;
        case HAL_BUSY:
            return UartError::Busy;
        case HAL_ERROR:
        default:
            // Check specific error flags
            uint32_t error = HAL_UART_GetError(&uart_handle_);
            if (error & HAL_UART_ERROR_PE)
                return UartError::Parity;
            if (error & HAL_UART_ERROR_NE)
                return UartError::Noise;
            if (error & HAL_UART_ERROR_FE)
                return UartError::Framing;
            if (error & HAL_UART_ERROR_ORE)
                return UartError::Overrun;
            if (error & HAL_UART_ERROR_DMA)
                return UartError::DMA;
            return UartError::HardwareFault;
        }
    }

    void UartWrapper::handle_error(UartError error)
    {
        last_error_ = error;
        if (error_callback_)
        {
            error_callback_(error);
        }
    }

    size_t UartWrapper::get_instance_index(USART_TypeDef *instance)
    {
        if (instance == USART1)
            return 0;
        if (instance == USART2)
            return 1;
        if (instance == USART3)
            return 2;
        if (instance == UART4)
            return 3;
        if (instance == UART5)
            return 4;
        if (instance == USART6)
            return 5;
        if (instance == UART7)
            return 6;
        if (instance == UART8)
            return 7;
        return 8; // Invalid
    }

    // === Global Interrupt Handlers ===

    extern "C"
    {

        void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
        {
            size_t index = UartWrapper::get_instance_index(huart->Instance);
            if (index < 8 && UartWrapper::instances_[index])
            {
                UartWrapper *uart = UartWrapper::instances_[index];
                if (uart->tx_complete_callback_)
                {
                    uart->tx_complete_callback_();
                }
            }
        }

        void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
        {
            size_t index = UartWrapper::get_instance_index(huart->Instance);
            if (index < 8 && UartWrapper::instances_[index])
            {
                UartWrapper *uart = UartWrapper::instances_[index];
                if (uart->rx_complete_callback_)
                {
                    // For this example, we'll pass the internal buffer
                    // In a real implementation, you'd track the actual received data
                    std::span<const uint8_t> data(uart->internal_rx_buffer_, 1);
                    uart->rx_complete_callback_(data);
                }
            }
        }

        void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
        {
            size_t index = UartWrapper::get_instance_index(huart->Instance);
            if (index < 8 && UartWrapper::instances_[index])
            {
                UartWrapper *uart = UartWrapper::instances_[index];
                uint32_t error = HAL_UART_GetError(huart);

                UartError uart_error = UartError::HardwareFault;
                if (error & HAL_UART_ERROR_PE)
                    uart_error = UartError::Parity;
                else if (error & HAL_UART_ERROR_NE)
                    uart_error = UartError::Noise;
                else if (error & HAL_UART_ERROR_FE)
                    uart_error = UartError::Framing;
                else if (error & HAL_UART_ERROR_ORE)
                    uart_error = UartError::Overrun;
                else if (error & HAL_UART_ERROR_DMA)
                    uart_error = UartError::DMA;

                uart->handle_error(uart_error);
            }
        }

    } // extern "C"

} // namespace lumos