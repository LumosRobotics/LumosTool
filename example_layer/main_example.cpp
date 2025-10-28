#include "uart_wrapper.hpp"
#include <array>
#include <string>
#include <cstdio>

// Example system clock configuration (you would adapt this to your specific STM32H7 board)
void SystemClock_Config(void);

int main()
{
    // Initialize HAL Library
    HAL_Init();

    // Configure system clock
    SystemClock_Config();

    // === Example 1: Simple UART usage with factory method ===
    {
        // Create UART3 instance with default configuration (115200 baud)
        auto uart = lumos::UartWrapper::create_usart3();

        if (!uart.is_ready())
        {
            // Handle initialization error
            return -1;
        }

        // Send a welcome message
        uart.send("Hello from C++ UART Wrapper!\r\n");

        // Send formatted message
        uart.send_formatted("System Clock: %lu MHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000);

        // Simple echo loop
        std::array<uint8_t, 64> buffer{};
        while (true)
        {
            size_t bytes_received = uart.receive_until(buffer, '\r', 5000);
            if (bytes_received > 0)
            {
                uart.send("Echo: ");
                uart.send(std::span<const uint8_t>(buffer.data(), bytes_received));
                uart.send("\r\n");
            }
        }
    }

    // === Example 2: Custom configuration ===
    {
        lumos::UartWrapper uart(USART1);

        lumos::UartConfig config;
        config.baudrate = 9600;
        config.word_length = UART_WORDLENGTH_8B;
        config.stop_bits = UART_STOPBITS_2;
        config.parity = UART_PARITY_EVEN;
        config.hw_flow_control = UART_HWCONTROL_RTS_CTS;

        // Configure GPIO pins
        config.tx_port = GPIOA;
        config.tx_pin = GPIO_PIN_9;
        config.tx_alternate_function = GPIO_AF7_USART1;

        config.rx_port = GPIOA;
        config.rx_pin = GPIO_PIN_10;
        config.rx_alternate_function = GPIO_AF7_USART1;

        // RTS/CTS pins
        config.rts_port = GPIOA;
        config.rts_pin = GPIO_PIN_12;
        config.rts_alternate_function = GPIO_AF7_USART1;

        config.cts_port = GPIOA;
        config.cts_pin = GPIO_PIN_11;
        config.cts_alternate_function = GPIO_AF7_USART1;

        if (!uart.initialize(config))
        {
            // Handle error
            auto error = uart.get_last_error();
            // Handle specific error...
            return -1;
        }

        uart.send("Custom configured UART ready!\r\n");
    }

    // === Example 3: Asynchronous operation with callbacks ===
    {
        auto uart = lumos::UartWrapper::create_usart2();

        if (!uart.is_ready())
        {
            return -1;
        }

        // Set error callback
        uart.set_error_callback([](lumos::UartError error)
                                {
            // Handle error
            printf("UART Error: %d\r\n", static_cast<int>(error)); });

        // Async transmission with callback
        std::string message = "Async message!\r\n";
        uart.send_async(message, []()
                        {
            // Transmission complete callback
            printf("Transmission completed!\r\n"); });

        // Async reception with callback
        std::array<uint8_t, 32> rx_buffer{};
        uart.receive_async(rx_buffer, [](std::span<const uint8_t> data)
                           {
            // Reception complete callback
            printf("Received %zu bytes\r\n", data.size()); });

        // Main loop - let interrupts handle UART operations
        while (true)
        {
            HAL_Delay(1000);
            // Do other work...
        }
    }

    // === Example 4: DMA-based transmission ===
    {
        auto uart = lumos::UartWrapper::create_uart4();

        if (!uart.is_ready())
        {
            return -1;
        }

        // Large data buffer for DMA
        std::array<uint8_t, 1024> large_buffer{};

        // Fill buffer with test data
        for (size_t i = 0; i < large_buffer.size(); ++i)
        {
            large_buffer[i] = static_cast<uint8_t>(i % 256);
        }

        // Send using DMA
        uart.send_dma(large_buffer, []()
                      { printf("DMA transmission completed!\r\n"); });

        // Wait for completion
        uart.flush();
    }

    // === Example 5: Error handling and state management ===
    {
        auto uart = lumos::UartWrapper::create_uart5();

        if (!uart.is_ready())
        {
            return -1;
        }

        std::array<uint8_t, 16> test_buffer{};

        // Try to receive with timeout
        size_t received = uart.receive(test_buffer, 1000);

        if (received == 0)
        {
            auto error = uart.get_last_error();
            switch (error)
            {
            case lumos::UartError::Timeout:
                uart.send("Timeout occurred\r\n");
                break;
            case lumos::UartError::Overrun:
                uart.send("Buffer overrun\r\n");
                break;
            case lumos::UartError::Framing:
                uart.send("Framing error\r\n");
                break;
            default:
                uart.send_formatted("Error: %d\r\n", static_cast<int>(error));
                break;
            }
        }

        // Check UART state
        auto state = uart.get_state();
        switch (state)
        {
        case lumos::UartState::Ready:
            uart.send("UART is ready\r\n");
            break;
        case lumos::UartState::Busy:
            uart.send("UART is busy\r\n");
            break;
        case lumos::UartState::Error:
            uart.send("UART in error state\r\n");
            break;
        default:
            uart.send_formatted("UART state: %d\r\n", static_cast<int>(state));
            break;
        }
    }

    return 0;
}

/**
 * @brief System Clock Configuration for STM32H7
 * @note This is a basic configuration - adapt to your specific board
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Supply configuration update enable
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

    // Configure the main internal regulator output voltage
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY))
    {
    }

    // Initialize the RCC Oscillators according to the specified parameters
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;
    RCC_OscInitStruct.PLL.PLLN = 220;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        while (1)
            ; // Error handler
    }

    // Initializes the CPU, AHB and APB buses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        while (1)
            ; // Error handler
    }
}

// Required for HAL_Delay
extern "C" void SysTick_Handler(void)
{
    HAL_IncTick();
}