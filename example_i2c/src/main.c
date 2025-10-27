#include "i2c_config.h"
#include <stdio.h>

// Buffer for sprintf
static char msg_buffer[128];

// Array to store found I2C devices
static uint8_t found_devices[128];
static uint8_t device_count = 0;

int main(void)
{
    // Initialize HAL Library
    HAL_Init();

    // Configure system clock to 550 MHz
    SystemClock_Config();

    // Initialize I2C
    I2C_Init();

    // Give I2C bus time to stabilize
    HAL_Delay(100);

    // Main loop
    while (1)
    {
        // Scan I2C bus for devices
        device_count = 0;
        I2C_ScanBus(found_devices, &device_count);

        // Display results
        if (device_count == 0)
        {
            // No devices found
            // In a real application, you might output this to UART or a display
            // For now, we just wait and try again
        }
        else
        {
            // Devices found
            // You can add code here to:
            // 1. Output results to UART
            // 2. Read from specific sensor (e.g., temperature sensor at 0x48)
            // 3. Control I2C peripherals

            // Example: If we find a device at 0x48 (common for temp sensors like LM75)
            for (uint8_t i = 0; i < device_count; i++)
            {
                if (found_devices[i] == 0x48)
                {
                    // Read temperature from LM75-compatible sensor
                    uint8_t temp_data[2];
                    HAL_StatusTypeDef result = I2C_ReadMulti(0x48, 0x00, temp_data, 2);

                    if (result == HAL_OK)
                    {
                        // Convert to temperature (LM75 format: 11-bit, 0.125°C per LSB)
                        int16_t raw_temp = (temp_data[0] << 8) | temp_data[1];
                        raw_temp >>= 5;  // Shift to get 11-bit value

                        // Convert to Celsius
                        float temperature = raw_temp * 0.125f;

                        // Store result (could be output to UART, display, etc.)
                        (void)temperature;  // Suppress unused variable warning
                    }
                }
            }
        }

        // Wait 1 second before next scan
        HAL_Delay(1000);
    }

    return 0;
}

// SysTick interrupt handler (required by HAL_Delay)
void SysTick_Handler(void)
{
    HAL_IncTick();
}
