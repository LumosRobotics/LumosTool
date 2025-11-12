#pragma once

/**
 * @file comparison_example.hpp
 * @brief Side-by-side comparison of C HAL vs C++ Wrapper approaches
 *
 * This file demonstrates the differences between traditional STM32 HAL usage
 * and the modern C++ wrapper approach for common UART operations.
 */

#include "uart_wrapper.hpp"
#include "stm32h7xx_hal.h"
#include <array>
#include <string>

namespace lumos
{

    /**
     * @brief Comparison examples showing C HAL vs C++ Wrapper approaches
     */
    class UartComparisonExamples
    {
    public:
        // ========================================================================
        // EXAMPLE 1: Basic UART Initialization
        // ========================================================================

        /**
         * @brief Traditional C HAL approach for UART initialization
         */
        static bool initialize_uart_c_style()
        {
            static UART_HandleTypeDef huart3;

            // Manual clock enabling
            __HAL_RCC_GPIOD_CLK_ENABLE();
            __HAL_RCC_USART3_CLK_ENABLE();

            // Manual GPIO configuration
            GPIO_InitTypeDef GPIO_InitStruct = {0};
            GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
            HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

            // Manual UART configuration
            huart3.Instance = USART3;
            huart3.Init.BaudRate = 115200;
            huart3.Init.WordLength = UART_WORDLENGTH_8B;
            huart3.Init.StopBits = UART_STOPBITS_1;
            huart3.Init.Parity = UART_PARITY_NONE;
            huart3.Init.Mode = UART_MODE_TX_RX;
            huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
            huart3.Init.OverSampling = UART_OVERSAMPLING_16;
            huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
            huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
            huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

            if (HAL_UART_Init(&huart3) != HAL_OK)
            {
                return false;
            }

            // Manual FIFO configuration
            if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
            {
                return false;
            }
            if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
            {
                return false;
            }
            if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
            {
                return false;
            }

            return true;
        }

        /**
         * @brief Modern C++ wrapper approach for UART initialization
         */
        static UartWrapper initialize_uart_cpp_style()
        {
            auto uart = UartWrapper::create_usart3();
            // That's it! Clock enabling, GPIO config, and UART setup are automatic
            return uart;
        }

        // ========================================================================
        // EXAMPLE 2: Sending Data
        // ========================================================================

        /**
         * @brief Traditional C HAL approach for sending data
         */
        static bool send_data_c_style(UART_HandleTypeDef *huart, const char *message)
        {
            // Manual string length calculation
            size_t len = strlen(message);

            // Unsafe cast and manual timeout handling
            HAL_StatusTypeDef status = HAL_UART_Transmit(huart,
                                                         (uint8_t *)message,
                                                         len,
                                                         1000);

            // Manual error checking
            if (status != HAL_OK)
            {
                // Need to check specific error manually
                uint32_t error = HAL_UART_GetError(huart);
                if (error & HAL_UART_ERROR_ORE)
                {
                    // Handle overrun
                }
                if (error & HAL_UART_ERROR_PE)
                {
                    // Handle parity error
                }
                // ... more manual error checking
                return false;
            }

            return true;
        }

        /**
         * @brief Modern C++ wrapper approach for sending data
         */
        static bool send_data_cpp_style(UartWrapper &uart, std::string_view message)
        {
            // Type-safe, automatic length calculation, comprehensive error handling
            bool success = uart.send(message);

            if (!success)
            {
                // Structured error handling
                auto error = uart.get_last_error();
                switch (error)
                {
                case UartError::Timeout:
                    // Handle timeout specifically
                    break;
                case UartError::Overrun:
                    // Handle overrun specifically
                    break;
                    // ... other specific error handling
                }
            }

            return success;
        }

        // ========================================================================
        // EXAMPLE 3: Formatted Output
        // ========================================================================

        /**
         * @brief Traditional C HAL approach for formatted output
         */
        static bool send_formatted_c_style(UART_HandleTypeDef *huart,
                                           uint32_t counter,
                                           uint32_t frequency)
        {
            // Manual buffer management with potential overflow
            static char buffer[128];

            // Unsafe sprintf (potential buffer overflow)
            int len = sprintf(buffer,
                              "Counter: %lu, Freq: %lu MHz\r\n",
                              counter,
                              frequency / 1000000);

            if (len < 0)
            {
                return false; // sprintf error
            }

            // Manual transmission
            HAL_StatusTypeDef status = HAL_UART_Transmit(huart,
                                                         (uint8_t *)buffer,
                                                         len,
                                                         1000);

            return status == HAL_OK;
        }

        /**
         * @brief Modern C++ wrapper approach for formatted output
         */
        static bool send_formatted_cpp_style(UartWrapper &uart,
                                             uint32_t counter,
                                             uint32_t frequency)
        {
            // Safe, integrated formatted output
            return uart.send_formatted("Counter: %lu, Freq: %lu MHz\r\n",
                                       counter,
                                       frequency / 1000000);
        }

        // ========================================================================
        // EXAMPLE 4: Asynchronous Operations
        // ========================================================================

        /**
         * @brief Traditional C HAL approach for async operations
         */
        static bool start_async_receive_c_style(UART_HandleTypeDef *huart,
                                                uint8_t *buffer,
                                                size_t size)
        {
            // Manual callback setup (requires global state management)
            // Need to manually manage the relationship between handle and callback

            HAL_StatusTypeDef status = HAL_UART_Receive_IT(huart, buffer, size);

            if (status != HAL_OK)
            {
                return false;
            }

            // Callback handling requires global functions:
            // void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
            //     // Manual instance identification
            //     if (huart->Instance == USART3) {
            //         // Handle USART3 completion
            //         // Need global state to know what to do
            //     }
            // }

            return true;
        }

        /**
         * @brief Modern C++ wrapper approach for async operations
         */
        static bool start_async_receive_cpp_style(UartWrapper &uart,
                                                  std::span<uint8_t> buffer)
        {
            // Type-safe, lambda-based callback with automatic instance management
            return uart.receive_async(buffer, [](std::span<const uint8_t> received_data)
                                      {
            // Type-safe callback with automatic data passing
            printf("Received %zu bytes\n", received_data.size());
            
            // Process received data with full type safety
            for (uint8_t byte : received_data) {
                // Safe iteration over received data
                printf("0x%02X ", byte);
            }
            printf("\n"); });
        }

        // ========================================================================
        // EXAMPLE 5: Error Handling Comparison
        // ========================================================================

        /**
         * @brief Traditional C HAL error handling
         */
        static void handle_uart_errors_c_style(UART_HandleTypeDef *huart)
        {
            uint32_t error = HAL_UART_GetError(huart);

            // Manual bit field checking
            if (error != HAL_UART_ERROR_NONE)
            {
                if (error & HAL_UART_ERROR_PE)
                {
                    // Parity error - need to remember what this constant means
                    printf("Parity error\n");
                }
                if (error & HAL_UART_ERROR_NE)
                {
                    // Noise error - another magic constant
                    printf("Noise error\n");
                }
                if (error & HAL_UART_ERROR_FE)
                {
                    // Framing error
                    printf("Framing error\n");
                }
                if (error & HAL_UART_ERROR_ORE)
                {
                    // Overrun error
                    printf("Overrun error\n");
                }
                if (error & HAL_UART_ERROR_DMA)
                {
                    // DMA error
                    printf("DMA error\n");
                }

                // Manual error clearing
                __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_PEF);
                __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_FEF);
                __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_NEF);
                __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF);
            }
        }

        /**
         * @brief Modern C++ wrapper error handling
         */
        static void handle_uart_errors_cpp_style(UartWrapper &uart)
        {
            auto error = uart.get_last_error();

            // Type-safe enumeration with clear meaning
            switch (error)
            {
            case UartError::None:
                // No error
                break;
            case UartError::Parity:
                printf("Parity error detected\n");
                break;
            case UartError::Noise:
                printf("Noise error detected\n");
                break;
            case UartError::Framing:
                printf("Framing error detected\n");
                break;
            case UartError::Overrun:
                printf("Buffer overrun detected\n");
                break;
            case UartError::DMA:
                printf("DMA error detected\n");
                break;
            case UartError::Timeout:
                printf("Operation timed out\n");
                break;
            case UartError::Busy:
                printf("UART is busy\n");
                break;
            case UartError::InvalidParameter:
                printf("Invalid parameter provided\n");
                break;
            case UartError::HardwareFault:
                printf("Hardware fault detected\n");
                break;
            }

            // Error clearing is handled automatically by the wrapper
        }

        // ========================================================================
        // EXAMPLE 6: Resource Management
        // ========================================================================

        /**
         * @brief Traditional C HAL resource management issues
         */
        static void resource_management_c_style()
        {
            // Global or static variables for UART handles
            static UART_HandleTypeDef huart1;
            static UART_HandleTypeDef huart2;

            // Manual initialization with error-prone setup
            // ... initialization code ...

            // Easy to forget cleanup
            // If function exits early due to error, resources may leak

            // Manual deinitialization required
            HAL_UART_DeInit(&huart1);
            HAL_UART_DeInit(&huart2);

            // Manual clock disabling (often forgotten)
            __HAL_RCC_USART1_CLK_DISABLE();
            __HAL_RCC_USART2_CLK_DISABLE();

            // GPIO deinitialization (also often forgotten)
            HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);
            HAL_GPIO_DeInit(GPIOD, GPIO_PIN_5 | GPIO_PIN_6);
        }

        /**
         * @brief Modern C++ wrapper resource management
         */
        static void resource_management_cpp_style()
        {
            // RAII - automatic resource management
            {
                auto uart1 = UartWrapper::create_usart1();
                auto uart2 = UartWrapper::create_usart2();

                // Use UARTs...
                uart1.send("Hello from UART1\n");
                uart2.send("Hello from UART2\n");

                // Resources automatically cleaned up when objects go out of scope
                // No manual deinitialization required
                // Clocks automatically disabled
                // GPIO automatically deinitialized
            } // <- Automatic cleanup happens here

            // Can also be explicit about cleanup
            auto uart3 = UartWrapper::create_usart3();
            // ... use uart3 ...
            uart3.deinitialize(); // Explicit cleanup if needed
        }

        // ========================================================================
        // SUMMARY OF DIFFERENCES
        // ========================================================================

        /**
         * @brief Summary of key differences between approaches
         */
        static void print_comparison_summary()
        {
            printf("\n=== C HAL vs C++ Wrapper Comparison ===\n\n");

            printf("Configuration:\n");
            printf("  C HAL:     Manual clock enable, GPIO setup, parameter configuration\n");
            printf("  C++ Wrapper: Single factory method call with automatic setup\n\n");

            printf("Type Safety:\n");
            printf("  C HAL:     Void pointers, manual casting, size parameters\n");
            printf("  C++ Wrapper: std::span, std::string_view, automatic size deduction\n\n");

            printf("Error Handling:\n");
            printf("  C HAL:     Manual bit field checking, magic constants\n");
            printf("  C++ Wrapper: Type-safe enums, structured error handling\n\n");

            printf("Resource Management:\n");
            printf("  C HAL:     Manual init/deinit, easy to leak resources\n");
            printf("  C++ Wrapper: RAII, automatic cleanup, exception safety\n\n");

            printf("Async Operations:\n");
            printf("  C HAL:     Global callbacks, manual instance management\n");
            printf("  C++ Wrapper: Lambda callbacks, automatic instance tracking\n\n");

            printf("Memory Safety:\n");
            printf("  C HAL:     Buffer overflows, manual size tracking\n");
            printf("  C++ Wrapper: Bounds checking, type-safe containers\n\n");

            printf("Code Readability:\n");
            printf("  C HAL:     Verbose, repetitive, error-prone\n");
            printf("  C++ Wrapper: Concise, expressive, self-documenting\n\n");
        }
    };

} // namespace lumos