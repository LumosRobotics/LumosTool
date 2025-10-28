#include "stm32h7xx_hal.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include <string.h>
#include <stdio.h>

// Test message to send periodically
const char* test_message = "Hello from STM32H7 USB CDC!\r\n";

int main(void)
{
    uint32_t last_send_time = 0;
    char buffer[64];
    uint32_t counter = 0;

    // Initialize HAL Library
    HAL_Init();

    // Configure system clock to 550 MHz
    SystemClock_Config();

    // Initialize USB Device (CDC Virtual COM Port)
    USB_Device_Init();

    // Give USB device time to enumerate
    HAL_Delay(1000);

    // Main loop
    while (1)
    {
        // Send a message every 1 second
        if (HAL_GetTick() - last_send_time >= 1000)
        {
            last_send_time = HAL_GetTick();

            // Create a message with counter
            snprintf(buffer, sizeof(buffer), "Message #%lu: %s", counter++, test_message);

            // Transmit via USB CDC
            CDC_Transmit_FS((uint8_t*)buffer, strlen(buffer));
        }

        // Small delay to avoid busy-waiting
        HAL_Delay(10);
    }

    return 0;
}

/**
 * @brief System Clock Configuration
 * @note Configures the system to run at 550 MHz (max for H7)
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    // Supply configuration update enable
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

    // Configure the main internal regulator output voltage
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    // Configure HSE (High Speed External oscillator)
    // Assuming 25 MHz external crystal (common on H7 boards)
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI48;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;  // Enable HSI48 for USB
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

    // PLL1 configuration for 550 MHz
    // HSE = 25 MHz
    // VCO = (HSE / PLLM) * PLLN = (25 / 5) * 220 = 1100 MHz
    // SYSCLK = VCO / PLLP = 1100 / 2 = 550 MHz
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
        Error_Handler();
    }

    // Configure CPU, AHB and APB buses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                  RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;      // 275 MHz
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;     // 137.5 MHz
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;     // 137.5 MHz
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;     // 137.5 MHz
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;     // 137.5 MHz

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }

    // Configure USB clock source to use HSI48 (48 MHz required for USB)
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief Error handler - infinite loop
 */
void Error_Handler(void)
{
    // Disable interrupts
    __disable_irq();

    // Infinite loop
    while (1)
    {
        // Optionally toggle an LED here if available
    }
}

// SysTick interrupt handler (required by HAL_Delay)
void SysTick_Handler(void)
{
    HAL_IncTick();
}
